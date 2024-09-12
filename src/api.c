#include "errmsg.h"
#include "hbs.h"

#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "state.h"
#include "val.h"
#include "obj.h"
#include "tostr.h"

static Val val_at(hbs_State* h, int index) {
  if (index >= 0) {
    Val* val = h->frame->base + index;
    if (val > h->top) {
      hbs_err(h, "Invalid stack access of slot %d (C API)", index);
    }
    return *val;
  }

  return *(h->top + index);
}

static GcStr* get_name_or(hbs_State* h, Val val, const char* name) {
  if (name != NULL) {
    return copy_str(h, name, strlen(name));
  } else if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_struct: return as_struct(val)->name; break;
      case obj_enum: return as_enum(val)->name; break;
      case obj_c_fn: return as_c_fn(val)->name; break;
      case obj_fn: return as_fn(val)->name; break;
      default: break;
    }
  }

  hbs_err(h, "Must provide a name for global (C API)");
  return NULL;
}

static GcStr* obj_to_string(hbs_State* h, int index) {
  Val val = val_at(h, index);

  if (is_inst(val) && hbs_has_prop(h, "tostr", index)) {
    hbs_push(h, index);
    hbs_callon(h, "tostr", 0);
    if (!hbs_is_string(h, -1)) {
      hbs_err(h, "expected string to be returned from 'tostr'");
    }

    return as_str(pop(h));
  }
  return to_str(h, val);
}


void hbs_expect_type(hbs_State* h, int index, hbs_ValueType expect) {
  hbs_ValueType type = hbs_get_type(h, index);
  if (type != expect) {
    hbs_err(
      h, "expected '%s', got '%s'",
      hbs_typestr(expect, NULL), hbs_typestr(type, NULL));
  }
}

void hbs_pop(hbs_State* h, int c) {
  h->top -= c;
}

void hbs_push(hbs_State* h, int index) {
  push(h, val_at(h, index));
}

void hbs_set_global(hbs_State* h, const char* name) {
  Val val = val_at(h, -1);

  GcStr* sname = get_name_or(h, val, name);
  push(h, create_obj(sname));
  set_map(h, &h->globals, sname, val);

  hbs_pop(h, 2);
}

void hbs_get_global(hbs_State* h, const char* name) {
  Val val;
  if (get_map(&h->globals, copy_str(h, name, strlen(name)), &val)) {
    push(h, val);
    return;
  }
  hbs_err(h, "No global named '%s' (C API)", name);
}

hbs_ValueType hbs_get_type(hbs_State* h, int index) {
  Val val = val_at(h, index);

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
      case obj_enum: return hbs_type_enum;
      case obj_method:
      case obj_closure: return hbs_type_function;
      case obj_c_fn: return hbs_type_cfunction;
      case obj_inst: return hbs_type_instance;
      case obj_arr: return hbs_type_array;
      default: break;
    }
  }

  hbs_err(h, "Internal value type, which should not be accessible (C API)");
  return 0;
}


bool hbs_has_prop(hbs_State* h, const char* name, int index) {
  Val inst_val = val_at(h, index);
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

void hbs_get_prop(hbs_State* h, const char* name, int index) {
  Val inst_val = val_at(h, index);
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


double hbs_get_num(hbs_State* h, int index) {
  hbs_check_num(h, index);
  Val val = val_at(h, index);
  return as_num(val);
}

bool hbs_get_bool(hbs_State* h, int index) {
  Val val = val_at(h, index);
  return !(is_null(val) || (is_bool(val) && !as_bool(val)));
}

hbs_CFn hbs_get_cfunction(hbs_State* h, int index) {
  hbs_check_cfunction(h, index);
  Val val = val_at(h, index);
  return as_c_fn(val)->fn;
}

const char* hbs_get_string(hbs_State* h, int index, size_t* len_out) {
  hbs_check_string(h, index);
  Val val = val_at(h, index);

  GcStr* str = as_str(val);
  if (len_out != NULL) {
    *len_out = str->len;
  }

  return str->chars;
}

const char* hbs_get_tostring(hbs_State* h, int index, size_t* len_out) {
  GcStr* str = obj_to_string(h, index);
  if (len_out != NULL) {
    *len_out = str->len;
  }

  return str->chars;
}


void hbs_push_num(hbs_State* h, double num) {
  push(h, create_num(num));
}

void hbs_push_bool(hbs_State* h, bool b) {
  push(h, create_bool(b));
}

void hbs_push_null(hbs_State* h) {
  push(h, create_null());
}

void hbs_push_struct(hbs_State* h, const char* name) {
  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));
  GcStruct* s = create_struct(h, str_name);
  pop(h);
  push(h, create_obj(s));
}

void hbs_push_enum(hbs_State* h, const char* name) {
  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));
  GcEnum* s = create_enum(h, str_name);
  pop(h);
  push(h, create_obj(s));
}

void hbs_push_cfunction(hbs_State* h, const char* name, hbs_CFn fn, int argc) {
  GcStr* s = copy_str(h, name, strlen(name));
  push(h, create_obj(s));
  GcCFn* cfn = create_c_fn(h, s, fn, argc);
  pop(h);
  push(h, create_obj(cfn));
}

void hbs_push_string_copy(hbs_State* h, const char* chars, size_t len) {
  push(h, create_obj(copy_str(h, chars, len)));
}

void hbs_push_string(hbs_State* h, char* chars, size_t len) {
  push(h, create_obj(take_str(h, chars, len)));
}


const char* hbs_typestr(hbs_ValueType type, size_t* len_out) {
  size_t dummy;
  if (len_out == NULL) {
    len_out = &dummy;
  }

  switch (type) {
    case hbs_type_number:
      *len_out = 6;
      return "number";
    case hbs_type_bool:
      *len_out = 7;
      return "boolean";
    case hbs_type_null:
      *len_out = 5;
      return "tnull";
    case hbs_type_string:
      *len_out = 6;
      return "string";
    case hbs_type_instance:
      *len_out = 8;
      return "instance";
    case hbs_type_struct:
      *len_out = 7;
      return "tstruct";
    case hbs_type_enum:
      *len_out = 4;
      return "enum";
    case hbs_type_function:
      *len_out = 8;
      return "function";
    case hbs_type_cfunction:
      *len_out = 10;
      return "cfunction";
    case hbs_type_array:
      *len_out = 5;
      return "array";
    default: break;
  }

  *len_out = 7;
  return "unknown";
}

void hbs_tostr(hbs_State* h, int index) {
  push(h, create_obj(obj_to_string(h, index)));
}


void hbs_call(hbs_State* h, int argc) {
  vm_call(h, h->top[-1 - argc], argc);
}

void hbs_callon(hbs_State* h, const char* mname, int argc) {
  vm_invoke(h, copy_str(h, mname, strlen(mname)), argc);
}

void hbs_open_lib(hbs_State* h, hbs_CFn fn) {
  hbs_push_cfunction(h, "open_lib", fn, 0);
  hbs_call(h, 0);
  hbs_pop(h, 1);
}


void hbs_add_static_const(hbs_State* h, const char* name, int _struct) {
  Val s_val = val_at(h, _struct);
  if (!is_struct(s_val)) {
    hbs_err(h, "Expected struct");
    return;
  }

  GcStruct* s = as_struct(s_val);
  Val val = val_at(h, -1);
  GcStr* str_name = get_name_or(h, val, name);
  push(h, create_obj(str_name));
  set_map(h, &s->staticm, str_name, val);
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
    hbs_push_cfunction(h, method->name, method->fn, method->argc);
    hbs_add_member(h, method->mtype, _struct);
  }
}

void hbs_add_enum(hbs_State* h, const char* name, int _enum) {
  hbs_check_enum(h, _enum);

  GcEnum* e = as_enum(val_at(h, _enum));
  GcStr* sname = copy_str(h, name, strlen(name));
  push(h, create_obj(sname));
  int i = e->vals.count;
  if (!set_map(h, &e->vals, sname, create_num(i))) {
    hbs_err(h, err_msg_shadow_prev_enum, name);
  }
  pop(h); // sname
}
