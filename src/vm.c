#include "vm.h"

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "arr.h"
#include "chunk.h"
#include "parser.h"
#include "common.h"
#include "errmsg.h"
#include "hby.h"
#include "table.h"
#include "map.h"
#include "mem.h"
#include "obj.h"
#include "state.h"
#include "tostr.h"
#include "val.h"

#ifdef hby_trace_exec
# include "debug.h"
#endif

static void show_err(hby_State* h, const char* fmt, va_list args) {
  fprintf(stderr, "Stack trace (error source is first):\n");

  for (CallFrame* frame = h->frame; frame > h->frame_stack; frame--) {
    switch (frame->type) {
      case call_type_c:
        fprintf(stderr, "\t[C] %s()\n", frame->fn.c->name->chars);
        break;
      case call_type_hby:
      case call_type_capi: {
        GcFn* fn = frame->fn.hby->fn;
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

void hby_err(hby_State* h, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  show_err(h, fmt, args);
  va_end(args);

  longjmp(h->err_jmp->buf, hby_res_runtime_err);
}

static Val peek(hby_State* h, int dist) {
  return h->top[-1 - dist];
}

static bool call_fn(hby_State* h, GcClosure* closure, int argc) {
  if (argc != closure->fn->arity) {
    hby_err(h, err_msg_bad_argc, closure->fn->arity, argc);
    return false;
  }
  
  if (h->frame - h->frame_stack == frames_max) {
    hby_err(h, err_msg_stack_overflow);
    return false;
  }

  CallFrame* frame = ++h->frame;
  frame->fn.hby = closure;
  frame->ip = closure->fn->chunk.code;
  frame->type = call_type_hby;
  frame->base = h->top - argc - 1;
  return true;
}

static bool call_c(hby_State* h, GcCFn* c_fn, int argc) {
  if (c_fn->arity != -1 && argc != c_fn->arity) {
    hby_err(h, err_msg_bad_argc, c_fn->arity, argc);
    return false;
  }

  if (h->frame - h->frame_stack == frames_max) {
    hby_err(h, err_msg_stack_overflow);
    return false;
  }

  // TODO: To support pcalls, remove this error handling?
  // All errors should now jump straight to some point with the result
  
  CallFrame* frame = ++h->frame;
  frame->fn.c = c_fn;
  frame->ip = NULL;
  frame->type = call_type_c;
  frame->base = h->top - argc - 1;
  Val val = c_fn->fn(h, argc) ? pop(h) : create_null();

  h->top = h->frame->base;
  push(h, val);
  h->frame--;

  return true;
}

bool call_val(hby_State* h, Val val, int argc) {
  if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_method: {
        GcMethod* method = as_method(val);
        h->top[-argc - 1] = method->owner;
        if (method_type(method) == obj_closure)
          return call_fn(h, method->fn.hby, argc);
        return call_c(h, method->fn.c, argc);
      }
      case obj_closure: return call_fn(h, as_closure(val), argc);
      case obj_c_fn: return call_c(h, as_c_fn(val), argc);
      default: break;
    }
  }
  hby_err(h, err_msg_bad_callee);
  return false;
}

bool builtin_invoke(hby_State* h, GcStruct* _struct, GcStr* name, int argc) {
  Val method;
  if (!get_table(&_struct->methods, name, &method)) {
    hby_err(h, err_msg_undef_prop, name->chars);
    return false;
  }
  return call_c(h, as_c_fn(method), argc);
}

bool invoke(hby_State* h, GcStr* name, int argc) {
  Val reciever = peek(h, argc);

  if (is_obj(reciever)) {
    switch (obj_type(reciever)) {
      case obj_inst: {
        GcInst* inst = as_inst(reciever);

        Val val;
        if (get_table(&inst->fields, name, &val)) {
          h->top[-argc - 1] = val;
          return call_val(h, val, argc);
        }

        Val method;
        if (!get_table(&inst->_struct->methods, name, &method)) {
          hby_err(h, err_msg_undef_prop, name->chars);
          return false;
        }

        if (is_c_fn(method)) {
          return call_c(h, as_c_fn(method), argc);
        }
        return call_fn(h, as_closure(method), argc);
      }
      case obj_arr:
        return builtin_invoke(h, h->array_struct, name, argc);
      case obj_map:
        return builtin_invoke(h, h->map_struct, name, argc);
      case obj_str:
        return builtin_invoke(h, h->string_struct, name, argc);
      case obj_udata: {
        GcUData* udata = as_udata(reciever);
        
        if (udata->metastruct == NULL) {
          hby_err(h, err_msg_undef_prop, name->chars);
          return false;
        }
        
        Val cfn;
        if (!get_table(&udata->metastruct->methods, name, &cfn)) {
          hby_err(h, err_msg_undef_prop, name->chars);
          return false;
        }
        
        return call_c(h, as_c_fn(cfn), argc);
      }
      default:
        break;
    }
  }

  hby_err(h, err_msg_bad_prop_access);
  return false;
}

static bool static_access(hby_State* h, Val val, GcStr* prop) {
  if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_struct: {
        GcStruct* s = as_struct(val);

        Val val;
        if (get_table(&s->staticm, prop, &val)) {
          pop(h); // Struct
          push(h, val);
          return true;
        }

        hby_err(h, err_msg_undef_static_prop, prop->chars);
        return false;
      }
      case obj_enum: {
        GcEnum* e = as_enum(val);

        Val val;
        if (get_table(&e->vals, prop, &val)) {
          pop(h); // Enum
          push(h, val);
          return true;
        }

        hby_err(h, err_msg_undef_static_prop, prop->chars);
        return false;
      }
      default: break;
    }
  }

  hby_err(h, err_msg_bad_static_access);
  return false;
}

static bool subscript_get(hby_State* h, Val container, Val k) {
  if (is_obj(container)) {
    switch (obj_type(container)) {
      case obj_arr: {
        if (!is_num(k)) {
          hby_err(h, err_msg_bad_operand("number"));
          return false;
        }

        int idx = (int)as_num(k);
        GcArr* arr = as_arr(container);
        if (idx < 0) {
          idx += arr->varr.len;
        }

        if (idx < 0 || idx >= arr->varr.len) {
          hby_err(h, err_msg_index_out_of_bounds);
          return false;
        }
        push(h, arr->varr.items[idx]);
        return true;
      }
      case obj_map: {
        GcMap* map = as_map(container);
        Val val;
        if (!get_map(map, k, &val)) {
          hby_err(h, err_msg_undef_map_key, to_str(h, k)->chars);
          return false;
        }
        push(h, val);
        return true;
      }
      case obj_str: {
        if (!is_num(k)) {
          hby_err(h, err_msg_bad_operand("number"));
          return false;
        }
        int idx = (int)as_num(k);
        GcStr* str = as_str(container);
        if (idx < 0) {
          idx += str->len;
        }

        if (idx < 0 || idx >= str->len) {
          hby_err(h, err_msg_index_out_of_bounds);
          return false;
        }
        push(h, create_obj(copy_str(h, &str->chars[idx], 1)));
        return true;
      }
      default:
        break;
    }
  }

  hby_err(h, err_msg_bad_sub_get);
  return false;
}

static bool subscript_set(hby_State* h, Val container, Val k, Val val) {
  if (is_obj(container)) {
    switch (obj_type(container)) {
      case obj_arr: {
        if (!is_num(k)) {
          hby_err(h, err_msg_bad_operand("number"));
          return false;
        }

        int idx = (int)as_num(k);
        GcArr* arr = as_arr(container);
        if (idx < 0) {
          idx += arr->varr.len;
        }

        if (idx < 0 || idx >= arr->varr.len) {
          hby_err(h, err_msg_index_out_of_bounds);
          return false;
        }
        arr->varr.items[idx] = val;
        return true;
      }
      case obj_map: {
        GcMap* map = as_map(container);
        set_map(h, map, k, val);
        return true;
      }
      default:
        break;
    }
  }

  hby_err(h, err_msg_bad_sub_set);
  return false;
}

static bool bind_method(hby_State* h, GcStruct* _struct, GcStr* name) {
  Val val;
  if (!get_table(&_struct->methods, name, &val)) {
    hby_err(h, err_msg_undef_prop, name->chars);
    return false;
  }

  GcMethod* method = NULL;
  if (is_closure(val)) {
    method = create_method(h, peek(h, 0), as_closure(val));
  } else if (is_c_fn(val)) {
    method = create_c_method(h, peek(h, 0), as_c_fn(val));
  }
  pop(h);
  push(h, create_obj(method));
  return true;
}

static GcUpval* capture_upval(hby_State* h, Val* local) {
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

static void close_upvals(hby_State* h, Val* last) {
  while (h->open_upvals != NULL && h->open_upvals->loc >= last) {
    GcUpval* upval = h->open_upvals;
    upval->closed = *upval->loc;
    upval->loc = &upval->closed;
    h->open_upvals = upval->next;
  }
}

static void define_method(hby_State* h, GcStr* name) {
  Val method = peek(h, 0);
  GcStruct* s = as_struct(peek(h, 1));
  set_table(h, &s->methods, name, method);
  pop(h);
}

static bool is_false(Val val) {
  return is_null(val) || (is_bool(val) && !as_bool(val));
}

GcStruct* vm_get_typestruct(hby_State* h, Val val) {
  if (is_num(val)) {
    return h->number_struct;
  } else if (is_bool(val)) {
    return h->boolean_struct;
  } else if (is_closure(val) || is_method(val)) {
    return h->function_struct;
  } else if (is_inst(val)) {
    return as_inst(val)->_struct;
  } else if (is_arr(val)) {
    return h->array_struct;
  } else if (is_str(val)) {
    return h->string_struct;
  } else if (is_struct(val)) {
    return as_struct(val);
  } else if (is_udata(val)) {
    GcUData* udata = as_udata(val);
    if (udata->metastruct == NULL) {
      return h->udata_struct;
    }
    return udata->metastruct;
  }

  return NULL;
}

void vm_concat(hby_State* h) {
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

bool vm_isop(hby_State* h) {
  if (!is_struct(peek(h, 0))) {
    return false;
  }

  GcStruct* b = as_struct(pop(h));
  GcStruct* a = vm_get_typestruct(h, pop(h));
  if (a == NULL) {
    return false;
  }

  return a == b;
}

static bool get_property(hby_State* h, Val owner, GcStr* name) {
  if (is_obj(owner)) {
    switch (obj_type(owner)) {
      case obj_inst: {
        GcInst* inst = as_inst(owner);

        Val val;
        if (get_table(&inst->fields, name, &val)) {
          pop(h);
          push(h, val);
          return true;
        }

        if (!bind_method(h, inst->_struct, name)) {
          return false;
        }
        return true;
      }
      case obj_arr: {
        Val cfn;
        if (!get_table(&h->array_struct->methods, name, &cfn)) {
          hby_err(h, err_msg_undef_prop, name->chars);
          return false;
        }

        GcMethod* method = create_c_method(h, peek(h, 0), as_c_fn(cfn));
        pop(h);
        push(h, create_obj(method));
        return true;
      }
      case obj_udata: {
        GcUData* udata = as_udata(owner);
        
        if (udata->metastruct == NULL) {
          hby_err(h, err_msg_undef_prop, name->chars);
          return false;
        }
        
        Val cfn;
        if (!get_table(&udata->metastruct->methods, name, &cfn)) {
          hby_err(h, err_msg_undef_prop, name->chars);
          return false;
        }
        
        GcMethod* method = create_c_method(h, peek(h, 0), as_c_fn(cfn));
        pop(h);
        push(h, create_obj(method));
        return true;
      }
      default:
        break;
    }
  }

  hby_err(h, err_msg_bad_prop_access);
  return false;
}

#define op_add(a, b) (a + b)
#define op_sub(a, b) (a - b)
#define op_mul(a, b) (a * b)
#define op_div(a, b) (a / b)
#define op_mod(a, b) fmod(a, b)

static void run(hby_State* h) {
#define read_byte() (*h->frame->ip++)
#define read_short() \
  (h->frame->ip += 2, (u16)((h->frame->ip[-2] << 8) | h->frame->ip[-1]))
#define read_const() (h->frame->fn.hby->fn->chunk.consts.items[read_byte()])
#define read_str() as_str(read_const())
#define bin_op(op) \
    do { \
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) { \
          hby_err(h, err_msg_bad_operands("numbers")); \
          return; \
        } \
        double b = as_num(pop(h)); \
        double a = as_num(pop(h)); \
        push(h, create_num(op(a, b))); \
    } while (false)

  while (true) {
#ifdef hby_trace_exec
    printf("        ");
    for (Val* slot = h->stack; slot < h->top; slot++) {
      printf("[ ");
      printf("%s", to_str(h, *slot)->chars);
      printf(" ]");
    }
    printf("\n");
    print_bc(
      h, &h->frame->fn.hby->fn->chunk,
      (int)(h->frame->ip - h->frame->fn.hby->fn->chunk.code));
#endif

    u8 bc;

    switch (bc = read_byte()) {
      case bc_pop: pop(h); break;
      case bc_close_upval:
        close_upvals(h, h->top - 1);
        pop(h);
        break;
      case bc_get_global: {
        GcStr* name = read_str();
        Val v;
        if (!get_table(&h->globals, name, &v)) {
          hby_err(h, err_msg_undef_var, name->chars);
          return;
        }
        push(h, v);
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
        push(h, *h->frame->fn.hby->upvals[slot]->loc);
        break;
      }
      case bc_set_upval: {
        u8 slot = read_byte();
        *h->frame->fn.hby->upvals[slot]->loc = peek(h, 0);
        break;
      }
      case bc_push_prop: {
        // Difference between push_prop and get_prop is that this one leaves the
        // struct on the stack
        if (!is_inst(peek(h, 0))) {
          hby_err(h, err_msg_bad_prop_access);
          return;
        }

        GcInst* inst = as_inst(peek(h, 0));
        GcStr* name = read_str();

        Val val;
        if (get_table(&inst->fields, name, &val)) {
          push(h, val);
          break;
        }

        if (!bind_method(h, inst->_struct, name)) {
          return;
        }
        break;
      }
      case bc_get_prop: {
        Val val = peek(h, 0);
        GcStr* name = read_str();
        if (!get_property(h, val, name)) {
          return;
        }
        break;
      }
      case bc_set_prop: {
        if (!is_inst(peek(h, 1))) {
          hby_err(h, err_msg_bad_prop_access);
          return;
        }

        GcInst* inst = as_inst(peek(h, 1));
        GcStr* name = read_str();
        if (set_table(h, &inst->fields, name, peek(h, 0))) {
          hby_err(h, err_msg_undef_prop, name->chars);;
          return;
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
        if (set_table(h, &inst->fields, name, peek(h, 0))) {
          hby_err(h, err_msg_undef_prop, name->chars);
          return;
        }
        pop(h);
        break;
      }
      case bc_get_subscript: {
        Val k = pop(h);
        Val container = pop(h);
        if (!subscript_get(h, container, k)) {
          return;
        }
        break;
      }
      case bc_set_subscript: {
        Val val = pop(h);
        Val k = pop(h);
        Val container = pop(h);
        if (!subscript_set(h, container, k, val)) {
          return;
        }
        push(h, val);
        break;
      }
      case bc_push_subscript: {
        Val k = peek(h, 0);
        Val container = peek(h, 1);
        if (!subscript_get(h, container, k)) {
          return;
        }
        break;
      }
      case bc_destruct_array: {
        int idx = read_byte();

        if (is_null(peek(h, 0))) {
          push(h, create_null());
          break;
        }

        if (!is_arr(peek(h, 0))) {
          hby_err(h, err_msg_bad_destruct_val);
          return;
        }

        GcArr* arr = as_arr(peek(h, 0));

        if (idx >= arr->varr.len) {
          hby_err(h, err_msg_bad_destruct_len);
          return;
        }

        push(h, arr->varr.items[idx]);
        break;
      }
      case bc_get_static: {
        if (!static_access(h, peek(h, 0), read_str())) {
          return;
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
        push_varr(h, &arr->varr, peek(h, 0));
        pop(h);
        break;
      }
      case bc_map: {
        GcMap* map = create_map(h);
        push(h, create_obj(map));
        break;
      }
      case bc_map_item: {
        GcMap* map = as_map(peek(h, 2));
        Val val = peek(h, 0);
        Val key = peek(h, 1);
        set_map(h, map, key, val);
        pop(h); // val
        pop(h); // key
        break;
      }
      case bc_add: bin_op(op_add); break;
      case bc_sub: bin_op(op_sub); break;
      case bc_mul: bin_op(op_mul); break;
      case bc_div: bin_op(op_div); break;
      case bc_mod: bin_op(op_mod); break;
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
          hby_err(h, err_msg_bad_operands("numbers"));
          return;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_bool(a >= b));
        break;
      }
      case bc_lte: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          hby_err(h, err_msg_bad_operands("numbers"));
          return;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_bool(a <= b));
        break;
      }
      case bc_gt: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          hby_err(h, err_msg_bad_operands("numbers"));
          return;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_bool(a > b));
        break;
      }
      case bc_lt: {
        if (!is_num(peek(h, 0)) || !is_num(peek(h, 1))) {
          hby_err(h, err_msg_bad_operands("numbers"));
          return;
        }
        double b = as_num(pop(h));
        double a = as_num(pop(h));
        push(h, create_bool(a < b));
        break;
      }
      case bc_cat:
        vm_concat(h);
        break;
      case bc_is:
        if (!is_struct(peek(h, 0))) {
          hby_err(h, err_msg_bad_operands("type"));
          return;
        }
        push(h, create_bool(vm_isop(h)));
        break;
      case bc_neg:
        if (!is_num(peek(h, 0))) {
          hby_err(h, err_msg_bad_operand("number"));
          return;
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
          return;
        }
        break;
      }
      case bc_invoke: {
        GcStr* name = read_str();
        int argc = read_byte();
        if (!invoke(h, name, argc)) {
          return;
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
            closure->upvals[i] = h->frame->fn.hby->upvals[idx];
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
        if (frame->type == call_type_capi) {
          return;
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
        set_table(h, &s->staticm, read_str(), val);
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
        set_table(h, &s->members, read_str(), val);
        pop(h);
        break;
      }
      case bc_enum: {
        GcStr* name = read_str();
        push(h, create_obj(name));
        GcEnum* _enum = create_enum(h, name);
        pop(h);
        push(h, create_obj(_enum));

        u8 count = read_byte();
        for (int i = 0; i < count; i++) {
          GcStr* name = read_str();
          push(h, create_obj(name));
          if (!set_table(h, &_enum->vals, name, create_num(i))) {
            hby_err(h, err_msg_shadow_prev_enum, name->chars);
            return;
          }
          pop(h);
        }

        break;
      }
      case bc_inst: {
        if (!is_struct(peek(h, 0))) {
          hby_err(h, err_msg_bad_inst);
          return;
        }

        GcStruct* s = as_struct(peek(h, 0));
        h->top[-1] = create_obj(create_inst(h, s));
        break;
      }
    }
  }

#undef bin_op
#undef read_byte
#undef read_short
#undef read_const
#undef read_str
}

void vm_invoke(hby_State* h, GcStr* name, int argc) {
  invoke(h, name, argc);

  if (h->frame->ip != NULL) {
    h->frame->type = call_type_capi;
    run(h);
  }
}

void vm_call(hby_State* h, Val val, int argc) {
  call_val(h, val, argc);

  if (is_closure(val)) {
    h->frame->type = call_type_capi;
    run(h);
  }
}

hby_Res vm_pcall(hby_State* h, Val val, int argc) {
  LongJmp jmp;
  jmp.prev = h->err_jmp;
  h->err_jmp = &jmp;

  CallFrame* old_frame = h->frame;

  hby_Res res;
  if ((res = setjmp(jmp.buf)) == 0) {
    call_val(h, val, argc);

    if (is_closure(val)) {
      h->frame->type = call_type_capi;
      run(h);
    }

    // No error
    h->err_jmp = jmp.prev;
    return res;
  }

  // Error
  h->frame = old_frame;
  h->err_jmp = jmp.prev;

  push(h, create_null());
  return res;
}

