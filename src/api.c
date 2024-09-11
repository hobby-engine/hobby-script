#include "hbs.h"

#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "state.h"
#include "val.h"
#include "obj.h"
#include "tostr.h"

static Val val_at(hbs_State* h, int stack_pos) {
  if (stack_pos >= 0) {
    Val* val = h->frame->base + stack_pos;
    if (val > h->top) {
      hbs_err(h, "Invalid stack access of slot %d (C API)", stack_pos);
    }
    return *val;
  }

  return *(h->top + stack_pos);
}

void hbs_pop(hbs_State* h, int c) {
  h->top -= c;
}

void hbs_push(hbs_State* h, int stack_pos) {
  push(h, val_at(h, stack_pos));
}

void hbs_call(hbs_State* h, int argc) {
  vm_call(h, h->top[-1 - argc], argc);
}

void hbs_method_call(hbs_State* h, const char* mname, int argc) {
  vm_invoke(h, copy_str(h, mname, strlen(mname)), argc);
}

void hbs_get_prop(hbs_State* h, const char* name, int stack_pos) {
  Val inst_val = val_at(h, stack_pos);
  if (!is_inst(inst_val)) {
    hbs_err(h, "Expected instance for call to 'hbs_get_prop'");
  }

  GcInst* inst = as_inst(inst_val);

  GcStr* str_name = copy_str(h, name, strlen(name));

  Val val;
  if (get_map(&inst->fields, str_name, &val)) {
    push(h, val);
    return;
  }

  if (get_map(&inst->_struct->methods, str_name, &val)) {
    push(h, val);
    return;
  }

  hbs_err(h, "Undefined property '%s'", name);
}

bool hbs_has_prop(hbs_State* h, const char* name, int stack_pos) {
  Val inst_val = val_at(h, stack_pos);
  if (!is_inst(inst_val)) {
    hbs_err(h, "Expected instance for call to 'hbs_get_prop'");
  }

  GcInst* inst = as_inst(inst_val);

  GcStr* str_name = copy_str(h, name, strlen(name));

  Val val;
  if (get_map(&inst->fields, str_name, &val)) {
    return true;
  }

  if (get_map(&inst->_struct->methods, str_name, &val)) {
    return true;
  }

  return false;
}

void hbs_set_global(hbs_State* h, const char* name) {
  Val val = val_at(h, -1);

  GcStr* sname = copy_str(h, name, strlen(name));
  push(h, create_obj(sname));
  set_map(h, &h->globals, sname, val);
}

hbs_ValueType hbs_get_type(hbs_State* h, int stack_pos) {
  Val val = val_at(h, stack_pos);

  if (is_num(val)) {
    return hbs_type_number;
  } else if (is_bool(val)) {
    return hbs_type_bool;
  } else if (is_null(val)) {
    return hbs_type_null;
  } else if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_str: return hbs_type_string;
      case obj_struct: return hbs_type_struct;
      case obj_fn: return hbs_type_function;
      case obj_c_fn: return hbs_type_c_function;
      case obj_inst: return hbs_type_instance;
      default: break;
    }
  }

  hbs_err(h, "Internal value type, which should not be accessible (C API)");
  return 0;
}

void hbs_open_lib(hbs_State* h, hbs_CFn fn) {
  hbs_push_c_fn(h, "open_lib", fn, 0);
  hbs_call(h, 0);
  hbs_pop(h, 1);
}

bool hbs_is_num(hbs_State* h, int stack_pos) {
  return is_num(val_at(h, stack_pos));
}

void hbs_push_num(hbs_State* h, double num) {
  push(h, create_num(num));
}

double hbs_get_num(hbs_State* h, int stack_pos) {
  Val val = val_at(h, stack_pos);
  if (!is_num(val)) {
    hbs_err(h, "Expected number for arg #%d", stack_pos);
  }
  return as_num(val);
}

bool hbs_get_bool(hbs_State* h, int stack_pos) {
  Val val = val_at(h, stack_pos);
  return !(is_null(val) || (is_bool(val) && !as_bool(val)));
}

bool hbs_is_bool(hbs_State* h, int stack_pos) {
  return is_bool(val_at(h, stack_pos));
} 

void hbs_push_bool(hbs_State* h, bool b) {
  push(h, create_bool(b));
}

void hbs_push_null(hbs_State* h) {
  push(h, create_null());
}

hbs_CFn hbs_get_c_fn(hbs_State* h, int stack_pos) {
  Val val = val_at(h, stack_pos);
  if (!is_c_fn(val)) {
    hbs_err(h, "Expected c function for arg #%d", stack_pos);
  }
  return as_c_fn(val)->fn;
}

void hbs_push_c_fn(hbs_State* h, const char* name, hbs_CFn fn, int argc) {
  GcStr* s = copy_str(h, name, strlen(name));
  push(h, create_obj(s));
  GcCFn* cfn = create_c_fn(h, s, fn, argc);
  pop(h);
  push(h, create_obj(cfn));
}

static GcStr* obj_to_str(hbs_State* h, int stack_pos) {
  Val val = val_at(h, stack_pos);

  if (is_inst(val) && hbs_has_prop(h, "tostr", stack_pos)) {
    hbs_push(h, stack_pos);
    hbs_method_call(h, "tostr", 0);
    if (!hbs_is_str(h, -1)) {
      hbs_err(h, "Expected string to be returned from 'tostr'");
    }

    return as_str(pop(h));
  }
  return to_str(h, val);
}

void hbs_to_str(hbs_State* h, int stack_pos) {
  push(h, create_obj(obj_to_str(h, stack_pos)));
}

const char* hbs_get_str(hbs_State* h, int stack_pos, size_t* len_out) {
  Val val = val_at(h, stack_pos);
  if (!is_str(val)) {
    hbs_err(h, "Expected string for arg #%d", stack_pos);
    return NULL;
  }

  GcStr* str = as_str(val);
  if (len_out != NULL) {
    *len_out = str->len;
  }

  return str->chars;
}

const char* hbs_get_and_to_str(hbs_State* h, int stack_pos, size_t* len_out) {
  GcStr* str = obj_to_str(h, stack_pos);
  if (len_out != NULL) {
    *len_out = str->len;
  }

  return str->chars;
}

bool hbs_is_str(hbs_State* h, int stack_pos) {
  return is_str(val_at(h, stack_pos));
}

void hbs_push_str_copy(hbs_State* h, const char* chars, size_t len) {
  push(h, create_obj(copy_str(h, chars, len)));
}

void hbs_push_str(hbs_State* h, char* chars, size_t len) {
  push(h, create_obj(take_str(h, chars, len)));
}

bool hbs_is_inst(hbs_State* h, int stack_pos) {
  return is_inst(val_at(h, stack_pos));
}

void hbs_define_struct(hbs_State* h, const char* name) {
  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));
  GcStruct* s = create_struct(h, str_name);
  pop(h);
  push(h, create_obj(s));
}

void hbs_add_static_const(hbs_State* h, const char* name, int _struct) {
  Val s_val = val_at(h, _struct);
  if (!is_struct(s_val)) {
    hbs_err(h, "Expected struct");
    return;
  }

  GcStruct* s = as_struct(s_val);
  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));
  set_map(h, &s->staticm, str_name, val_at(h, -2));
  pop(h); // str_name
  pop(h); // constant
}

void hbs_add_member(hbs_State* h, hbs_MethodType type, int _struct) {
  Val s_val = val_at(h, _struct);
  if (!is_struct(s_val)) {
    hbs_err(h, "Expected struct");
    return;
  }

  Val cfn_val = val_at(h, -1);
  if (!is_c_fn(cfn_val)) {
    hbs_err(h, "Expected c function at the top of the stack");
    return;
  }

  GcStruct* s = as_struct(s_val);
  GcCFn* cfn = as_c_fn(cfn_val);
  
  switch (type) {
    case hbs_static_fn: {
      set_map(h, &s->staticm, cfn->name, create_obj(cfn));
      break;
    }
    case hbs_method: {
      set_map(h, &s->methods, cfn->name, create_obj(cfn));
      break;
    }
  }

  pop(h); // c fn
}

void hbs_add_members(hbs_State* h, hbs_StructMethod* members, int _struct) {
  for (hbs_StructMethod* method = members; method->name != NULL; method++) {
    hbs_push_c_fn(h, method->name, method->fn, method->argc);
    hbs_add_member(h, method->mtype, _struct);
  }
}
