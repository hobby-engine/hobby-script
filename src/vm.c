#include "vm.h"

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "chunk.h"
#include "compiler.h"
#include "common.h"
#include "errmsg.h"
#include "hbs.h"
#include "lexer.h"
#include "map.h"
#include "mem.h"
#include "obj.h"
#include "state.h"
#include "tostr.h"
#include "val.h"

#ifdef hbs_trace_exec
# include "debug.h"
#endif

static void show_err(hbs_State* h, const char* fmt, va_list args) {
  fprintf(stderr, "Stack trace (error source is first):\n");

  for (CallFrame* frame = h->frame; frame > h->frame_stack; frame--) {
    switch (frame->type) {
      case call_type_c:
        fprintf(stderr, "\t[C] %s()\n", frame->fn.c->name->chars);
        break;
      case call_type_hbs:
      case call_type_script:
      case call_type_reenter: {
        GcFn* fn = frame->fn.hbs->fn;
        size_t bc = frame->ip - fn->chunk.code - 1;
        fprintf(stderr, "\t%s:%d in ", fn->path->chars, fn->chunk.lines[bc]);

        if (fn->name != NULL) {
          fprintf(stderr, "%s()\n", fn->name->chars);
        } else {
          fprintf(stderr, "script\n");
        }
        break;
      }
    }
  }

  fputs("[error] ", stderr);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);

  reset_stack(h);
}

static void runtime_err(hbs_State* h, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  show_err(h, fmt, args);
  va_end(args);
}

void hbs_err(hbs_State* h, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  show_err(h, fmt, args);
  va_end(args);

  longjmp(h->err_jmp, 1);
}

static Val peek(hbs_State* h, int dist) {
  return h->top[-1 - dist];
}

static bool call_fn(hbs_State* h, GcClosure* closure, int argc) {
  if (argc != closure->fn->arity) {
    runtime_err(h, err_msg_bad_argc, closure->fn->arity, argc);
    return false;
  }
  
  if (h->frame - h->frame_stack == frames_max) {
    runtime_err(h, err_msg_stack_overflow);
    return false;
  }

  CallFrame* frame = ++h->frame;
  frame->fn.hbs = closure;
  frame->ip = closure->fn->chunk.code;
  frame->type = call_type_hbs;
  frame->base = h->top - argc - 1;
  return true;
}

static bool call_c(hbs_State* h, GcCFn* c_fn, int argc) {
  if (c_fn->arity != -1 && argc != c_fn->arity) {
    runtime_err(h, err_msg_bad_argc, c_fn->arity, argc);
    return false;
  }

  if (h->frame - h->frame_stack == frames_max) {
    runtime_err(h, err_msg_stack_overflow);
    return false;
  }


  if (setjmp(h->err_jmp) == 0) {
    CallFrame* frame = ++h->frame;
    frame->fn.c = c_fn;
    frame->ip = NULL;
    frame->type = call_type_c;
    frame->base = h->top - argc - 1;

    volatile Val val = c_fn->fn(h, argc) ? pop(h) : create_null();

    h->top = h->frame->base;
    push(h, val);
    h->frame--;

    return true;
  }
  return false;
}

bool call_val(hbs_State* h, Val val, int argc) {
  if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_method: {
        GcMethod* method = as_method(val);
        h->top[-argc - 1] = method->owner;
        return call_fn(h, method->fn, argc);
      }
      case obj_closure: return call_fn(h, as_closure(val), argc);
      case obj_c_fn: return call_c(h, as_c_fn(val), argc);
      default: break;
    }
  }
  runtime_err(h, err_msg_bad_callee);
  return false;
}

bool invoke(hbs_State* h, GcStr* name, int argc) {
  Val reciever = peek(h, argc);

  if (is_obj(reciever)) {
    switch (obj_type(reciever)) {
      case obj_inst: {
        GcInst* inst = as_inst(reciever);

        Val val;
        if (get_map(&inst->fields, name, &val)) {
          h->top[-argc - 1] = val;
          return call_val(h, val, argc);
        }

        Val method;
        if (!get_map(&inst->_struct->methods, name, &method)) {
          runtime_err(h, err_msg_undef_prop, name->chars);
          return false;
        }

        if (is_c_fn(method)) {
          return call_c(h, as_c_fn(method), argc);
        }
        return call_fn(h, as_closure(method), argc);
      }
      case obj_arr: {
        Val method;
        if (!get_map(&h->arr_methods, name, &method)) {
          runtime_err(h, err_msg_undef_prop, name->chars);
          return false;
        }
        return call_c(h, as_c_fn(method), argc);
      }
      default:
        break;
    }
  }

  runtime_err(h, err_msg_bad_static_access);
  return false;
}

static bool static_access(hbs_State* h, Val val, GcStr* prop) {
  if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_struct: {
        GcStruct* s = as_struct(val);

        Val val;
        if (get_map(&s->staticm, prop, &val)) {
          pop(h); // Struct
          push(h, val);
          return true;
        }

        runtime_err(h, err_msg_undef_static_prop, prop->chars);
        return false;
      }
      case obj_enum: {
        GcEnum* e = as_enum(val);

        Val val;
        if (get_map(&e->vals, prop, &val)) {
          pop(h); // Enum
          push(h, val);
          return true;
        }

        runtime_err(h, err_msg_undef_static_prop, prop->chars);
        return false;
      }
      default: break;
    }
  }

  runtime_err(h, err_msg_bad_static_access);
  return false;
}

static bool subscript_get(hbs_State* h, Val container, Val k) {
  if (is_obj(container)) {
    switch (obj_type(container)) {
      case obj_arr: {
        if (!is_num(k)) {
          runtime_err(h, err_msg_bad_operand("number"));
          return false;
        }

        int idx = (int)as_num(k);
        GcArr* arr = as_arr(container);
        if (idx < 0) {
          idx += arr->arr.len;
        }

        if (idx < 0 || idx >= arr->arr.len) {
          runtime_err(h, err_msg_index_out_of_bounds);
          return false;
        }
        push(h, arr->arr.items[idx]);
        return true;
      }
      default:
        break;
    }
  }

  runtime_err(h, err_msg_bad_sub_access);
  return false;
}

static bool subscript_set(hbs_State* h, Val container, Val k, Val val) {
  if (is_obj(container)) {
    switch (obj_type(container)) {
      case obj_arr: {
        if (!is_num(k)) {
          runtime_err(h, err_msg_bad_operand("number"));
          return false;
        }

        int idx = (int)as_num(k);
        GcArr* arr = as_arr(container);
        if (idx < 0) {
          idx += arr->arr.len;
        }

        if (idx < 0 || idx >= arr->arr.len) {
          runtime_err(h, err_msg_index_out_of_bounds);
          return false;
        }
        arr->arr.items[idx] = val;
        return true;
      }
      default:
        break;
    }
  }

  runtime_err(h, err_msg_bad_sub_access);
  return false;
}

static bool bind_method(hbs_State* h, GcStruct* _struct, GcStr* name) {
  Val val;
  if (!get_map(&_struct->methods, name, &val)) {
    runtime_err(h, err_msg_undef_prop, name->chars);
    return false;
  }

  GcMethod* method = create_method(h, peek(h, 0), as_closure(val));
  pop(h);
  push(h, create_obj(method));
  return true;
}

static GcUpval* capture_upval(hbs_State* h, Val* local) {
  // Check if we already have this value captured
  GcUpval* prev_up = NULL;
  GcUpval* upval = h->open_upvals;
  while (upval != NULL && upval->loc > local) {
    prev_up = upval;
    upval = upval->next;
  }

  if (upval != NULL && upval->loc == local) {
    return upval;
  }

  // We don't already have this upvalue, create a new one
  GcUpval* new_upval = create_upval(h, local);
  new_upval->next = upval;

  if (prev_up == NULL) {
    h->open_upvals = new_upval;
  } else {
    prev_up->next = new_upval;
  }

  return new_upval;
}

static void close_upvals(hbs_State* h, Val* last) {
  while (h->open_upvals != NULL && h->open_upvals->loc >= last) {
    GcUpval* upval = h->open_upvals;
    upval->closed = *upval->loc;
    upval->loc = &upval->closed;
    h->open_upvals = upval->next;
  }
}

static void define_method(hbs_State* h, GcStr* name) {
  Val method = peek(h, 0);
  GcStruct* s = as_struct(peek(h, 1));
  set_map(h, &s->methods, name, method);
  pop(h);
}

static bool is_false(Val val) {
  return is_null(val) || (is_bool(val) && !as_bool(val));
}

static void concat(hbs_State* h) {
  GcStr* b = to_str(h, peek(h, 0));
  push(h, create_obj(b));
  GcStr* a = to_str(h, peek(h, 2));
  push(h, create_obj(a));

  int len = a->len + b->len;
  char* chars = allocate(h, char, len + 1);
  memcpy(chars, a->chars, a->len);
  memcpy(chars + a->len, b->chars, b->len);
  chars[len] = '\0';

  GcStr* res = take_str(h, chars, len);
  
  pop(h); // Str A
  pop(h); // Str B
  pop(h); // Val A
  pop(h); // Val B
  push(h, create_obj(res));
}

static hbs_InterpretResult run(hbs_State* h) {
#define read_byte() (*h->frame->ip++)
#define read_short() \
  (h->frame->ip += 2, (u16)((h->frame->ip[-2] << 8) | h->frame->ip[-1]))
#define read_const() (h->frame->fn.hbs->fn->chunk.consts.items[read_byte()])
#define read_str() as_str(read_const())

  while (true) {
#ifdef hbs_trace_exec
    printf("        ");
    for (Val* slot = h->stack; slot < h->top; slot++) {
      printf("[ ");
      printf("%s", to_str(h, *slot)->chars);
      printf(" ]");
    }
    printf("\n");
    print_bc(
      h, &h->frame->fn.hbs->fn->chunk,
      (int)(h->frame->ip - h->frame->fn.hbs->fn->chunk.code));
#endif

    u8 bc;

    switch (bc = read_byte()) {
      case bc_pop: pop(h); break;
      case bc_close_upval:
        close_upvals(h, h->top - 1);
        pop(h);
        break;
      case bc_def_global: {
        GcStr* name = read_str();
        set_map(h, &h->globals, name, peek(h, 0));
        pop(h);
        break;
      }
      case bc_get_global: {
        GcStr* name = read_str();
        Val v;
        if (!get_map(&h->globals, name, &v)) {
          runtime_err(h, err_msg_undef_var, name->chars);
          return hbs_result_runtime_err;
        }
        push(h, v);
        break;
      }
      case bc_set_global: {
        GcStr* name = read_str();
        if (set_map(h, &h->globals, name, peek(h, 0))) {
          rem_map(&h->globals, name);
          runtime_err(h, err_msg_undef_var, name->chars);
          return hbs_result_runtime_err;
        }
        break;
      }
      case bc_get_local: {
        u8 slot = read_byte();
        push(h, h->frame->base[slot]);
        break;
      }
      case bc_set_local: {
        u8 slot = read_byte();
        h->frame->base[slot] = peek(h, 0);
        break;
      }
      case bc_get_upval: {
        u8 slot = read_byte();
        push(h, *h->frame->fn.hbs->upvals[slot]->loc);
        break;
      }
      case bc_set_upval: {
        u8 slot = read_byte();
        *h->frame->fn.hbs->upvals[slot]->loc = peek(h, 0);
        break;
      }
      case bc_push_prop: {
        // Difference between push_prop and get_prop is that this one leaves the
        // struct on the stack
        if (!is_inst(peek(h, 0))) {
          runtime_err(h, err_msg_bad_prop_access);
          return hbs_result_runtime_err;
        }

        GcInst* inst = as_inst(peek(h, 0));
        GcStr* name = read_str();

        Val val;
        if (get_map(&inst->fields, name, &val)) {
          push(h, val);
          break;
        }

        if (!bind_method(h, inst->_struct, name)) {
          return hbs_result_runtime_err;
        }
        break;
      }
      case bc_get_prop: {
        if (!is_inst(peek(h, 0))) {
          runtime_err(h, err_msg_bad_prop_access);
          return hbs_result_runtime_err;
        }

        GcInst* inst = as_inst(peek(h, 0));
        GcStr* name = read_str();

        Val val;
        if (get_map(&inst->fields, name, &val)) {
          pop(h);
          push(h, val);
          break;
        }

        if (!bind_method(h, inst->_struct, name)) {
          return hbs_result_runtime_err;
        }
        break;
      }
      case bc_set_prop: {
        if (!is_inst(peek(h, 1))) {
          runtime_err(h, err_msg_bad_prop_access);
          return hbs_result_runtime_err;
        }

        GcInst* inst = as_inst(peek(h, 1));
        GcStr* name = read_str();
        if (set_map(h, &inst->fields, name, peek(h, 0))) {
          runtime_err(h, err_msg_undef_prop, name->chars);;
          return hbs_result_runtime_err;
        }

        Val val = pop(h); // Value assigned
        pop(h); // Instance
        push(h, val);

        break;
      }
      case bc_init_prop: {
        // This value being an instance is already verified by `bc_inst`
        GcInst* inst = as_inst(peek(h, 1));
        GcStr* name = read_str();
        if (set_map(h, &inst->fields, name, peek(h, 0))) {
          runtime_err(h, err_msg_undef_prop, name->chars);
          return hbs_result_runtime_err;
        }
        pop(h);
        break;
      }
      case bc_get_subscript: {
        Val k = pop(h);
        Val container = pop(h);
        if (!subscript_get(h, container, k)) {
          return hbs_result_runtime_err;
        }
        break;
      }
      case bc_set_subscript: {
        Val val = pop(h);
        Val k = pop(h);
        Val container = pop(h);
        if (!subscript_set(h, container, k, val)) {
          return hbs_result_runtime_err;
        }
        push(h, val);
        break;
      }
      case bc_push_subscript: {
        Val k = peek(h, 0);
        Val container = peek(h, 1);
        if (!subscript_get(h, container, k)) {
          return hbs_result_runtime_err;
        }
        break;
      }
      case bc_get_static: {
        if (!static_access(h, peek(h, 0), read_str())) {
          return hbs_result_runtime_err;
        }
        break;
      }
      case bc_const: {
        push(h, read_const());
        break;
      }
      case bc_true:  push(h, create_bool(true)); break;
      case bc_false: push(h, create_bool(false)); break;
      case bc_null:  push(h, create_null()); break;
      case bc_array: {
        GcArr* arr = create_arr(h);
        push(h, create_obj(arr));
        break;
      }
      case bc_array_item: {
        // Compiler ensures this is an array
        GcArr* arr = as_arr(peek(h, 1));
        push_valarr(h, &arr->arr, peek(h, 0));
        pop(h);
        break;
      }
      case bc_add: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_num(a + b));
        break;
      }
      case bc_sub: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_num(a - b));
        break;
      }
      case bc_mul: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_num(a * b));
        break;
      }
      case bc_div: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_num(a / b));
        break;
      }
      case bc_mod: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_num(fmod(a, b)));
        break;
      }
      case bc_eql: {
        Val b = pop(h);
        Val a = pop(h);
        push(h, create_bool(vals_eql(a, b)));
        break;
      }
      case bc_neql: {
        Val b = pop(h);
        Val a = pop(h);
        push(h, create_bool(!vals_eql(a, b)));
        break;
      }
      case bc_gte: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_bool(a >= b));
        break;
      }
      case bc_lte: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_bool(a <= b));
        break;
      }
      case bc_gt: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_bool(a > b));
        break;
      }
      case bc_lt: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          runtime_err(h, err_msg_bad_operands("numbers"));
          return hbs_result_runtime_err;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_bool(a < b));
        break;
      }
      case bc_cat: {
        concat(h);
        break;
      }
      case bc_neg:
        if (!is_num(peek(h, 0))) {
          runtime_err(h, err_msg_bad_operand("number"));
          return hbs_result_runtime_err;
        }
        push(h, create_num(-as_num(pop(h))));
        break;
      case bc_not: push(h, create_bool(is_false(pop(h)))); break;
      case bc_ineq_jmp: {
        u16 jmp = read_short();
        Val b = pop(h);
        Val a = peek(h, 0);
        if (!vals_eql(a, b)) {
          h->frame->ip += jmp;
        }
        break;
      }
      case bc_false_jmp: {
        u16 jmp = read_short();
        if (is_false(peek(h, 0))) {
          h->frame->ip += jmp;
        }
        break;
      }
      case bc_jmp: {
        u16 jmp = read_short();
        h->frame->ip += jmp;
        break;
      }
      case bc_loop: {
        u16 jmp = read_short();
        h->frame->ip -= jmp;
        break;
      }
      case bc_call: {
        int argc = read_byte();
        if (!call_val(h, peek(h, argc), argc)) {
          return hbs_result_runtime_err;
        }
        break;
      }
      case bc_invoke: {
        GcStr* name = read_str();
        int argc = read_byte();
        if (!invoke(h, name, argc)) {
          return hbs_result_runtime_err;
        }
        break;
      }
      case bc_closure: {
        GcFn* fn = as_fn(read_const());
        GcClosure* closure = create_closure(h, fn);
        push(h, create_obj(closure));
        
        for (int i = 0; i < fn->upvalc; i++) {
          u8 is_local = read_byte();
          u8 idx = read_byte();
          if (is_local) {
            closure->upvals[i] = capture_upval(h, h->frame->base + idx);
          } else {
            closure->upvals[i] = h->frame->fn.hbs->upvals[idx];
          }
        }
        break;
      }
      case bc_ret: {
        Val res = pop(h);
        close_upvals(h, h->frame->base);

        h->top = h->frame->base;
        push(h, res);

        CallFrame* frame = h->frame--;
        if (frame->type == call_type_reenter || frame->type == call_type_script) {
          return hbs_result_ok;
        }

        break;
      }
      case bc_struct: {
        push(h, create_obj(create_struct(h, read_str())));
        break;
      }
      case bc_def_static: {
        Val val = peek(h, 0);
        GcStruct* s = as_struct(peek(h, 1));
        set_map(h, &s->staticm, read_str(), val);
        pop(h);
        break;
      }
      case bc_method: {
        define_method(h, read_str());
        break;
      }
      case bc_member: {
        Val val = peek(h, 0);
        GcStruct* s = as_struct(peek(h, 1));
        set_map(h, &s->members, read_str(), val);
        pop(h);
        break;
      }
      case bc_enum: {
        GcStr* name = read_str();
        push(h, create_obj(name));
        GcEnum* _enum = create_enum(h, name);
        pop(h);

        u8 count = read_byte();
        for (int i = 0; i < count; i++) {
          GcStr* name = read_str();
          push(h, create_obj(name));
          if (!set_map(h, &_enum->vals, name, create_num(i))) {
            runtime_err(h, err_msg_shadow_prev_enum, name->chars);
            return hbs_result_runtime_err;
          }
          pop(h);
        }

        push(h, create_obj(_enum));
        break;
      }
      case bc_inst: {
        if (!is_struct(peek(h, 0))) {
          runtime_err(h, err_msg_bad_inst);
          return hbs_result_runtime_err;
        }

        GcStruct* s = as_struct(peek(h, 0));
        h->top[-1] = create_obj(create_inst(h, s));
        break;
      }
    }
  }

#undef read_byte
#undef read_short
#undef read_const
#undef read_str
}

hbs_InterpretResult vm_interp(hbs_State* h, const char* path, const char* src) {
  GcFn* fn = compile_hbs(h, path, src);
  if (fn == NULL) {
    return hbs_result_compile_err;
  }
  
  push(h, create_obj(fn));
  GcClosure* closure = create_closure(h, fn);
  pop(h);
  push(h, create_obj(closure));
  call_fn(h, closure, 0);
  h->frame->type = call_type_script;

  hbs_InterpretResult res = run(h);

  set_map(h, &h->files, copy_str(h, path, strlen(path)), peek(h, 0));

  return res;
}

void vm_invoke(hbs_State* h, GcStr* name, int argc) {
  invoke(h, name, argc);

  if (h->frame->ip != NULL) {
    h->frame->type = call_type_reenter;
    run(h);
  }
}

void vm_call(hbs_State* h, Val val, int argc) {
  call_val(h, val, argc);

  if (is_fn(val)) {
    h->frame->type = call_type_reenter;
    run(h);
  }
}
