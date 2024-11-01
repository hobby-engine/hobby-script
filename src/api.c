#include "errmsg.h"
#include "hby.h"

#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "state.h"
#include "val.h"
#include "obj.h"
#include "tostr.h"

static Val val_at(hby_State* h, int index) {
  if (index >= 0) {
    Val* val = h->frame->base + index;
    if (val > h->top) {
      hby_err(h, "Invalid stack access of slot %d (C API)", index);
    }
    return *val;
  }

  return *(h->top + index);
}

static GcStr* get_name_or(hby_State* h, Val val, const char* name) {
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

  hby_err(h, "Must provide a name for function (C API)");
  return NULL;
}

static GcStr* obj_to_string(hby_State* h, int index) {
  Val val = val_at(h, index);

  if (is_inst(val) && hby_has_prop(h, "tostr", index)) {
    hby_push(h, index);
    hby_callon(h, "tostr", 0);
    if (!hby_is_str(h, -1)) {
      hby_err(h, "expected string to be returned from 'tostr'");
    }

    return as_str(pop(h));
  }

  if (is_udata(val) && hby_has_prop(h, "tostr", index)) {
    hby_push(h, index);
    hby_callon(h, "tostr", 0);

    if (!hby_is_str(h, -1)) {
      hby_err(h, "expected string to be returned from 'tostr'");
    }

    return as_str(pop(h));
  }
  return to_str(h, val);
}


void hby_expect_type(hby_State* h, int index, hby_ValueType expect) {
  hby_ValueType type = hby_get_type(h, index);
  if (type != expect) {
    hby_err(
      h, "expected '%s', got '%s'",
      hby_typestr(expect, NULL), hby_typestr(type, NULL));
  }
}

void hby_pop(hby_State* h, int c) {
  h->top -= c;
}

void hby_push(hby_State* h, int index) {
  push(h, val_at(h, index));
}

void hby_set_global(hby_State* h, const char* name) {
  Val val = val_at(h, -1);

  GcStr* sname = get_name_or(h, val, name);
  push(h, create_obj(sname));
  set_map(h, &h->globals, sname, val);

  hby_pop(h, 2);
}

void hby_get_global(hby_State* h, const char* name) {
  Val val;
  if (get_map(&h->globals, copy_str(h, name, strlen(name)), &val)) {
    push(h, val);
    return;
  }
  hby_err(h, "No global named '%s' (C API)", name);
}

hby_ValueType hby_get_type(hby_State* h, int index) {
  Val val = val_at(h, index);

  if (is_num(val)) {
    return hby_type_number;
  } else if (is_bool(val)) {
    return hby_type_bool;
  } else if (is_null(val)) {
    return hby_type_null;
  } else if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_str: return hby_type_string;
      case obj_struct: return hby_type_struct;
      case obj_enum: return hby_type_enum;
      case obj_method:
      case obj_closure: return hby_type_function;
      case obj_c_fn: return hby_type_cfunction;
      case obj_inst: return hby_type_instance;
      case obj_udata: return hby_type_udata;
      case obj_arr: return hby_type_array;
      default: break;
    }
  }

  hby_err(h, "Internal value type, which should not be accessible (C API)");
  return 0;
}


bool hby_has_prop(hby_State* h, const char* name, int index) {
  Val val = val_at(h, index);
  GcStr* str_name = copy_str(h, name, strlen(name));

  if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_inst: {
        GcInst* inst = as_inst(val);
        Val val;
        if (get_map(&inst->fields, str_name, &val)) {
          return true;
        }
        if (get_map(&inst->_struct->methods, str_name, &val)) {
          return true;
        }
        return false;
      }
      case obj_udata: {
        GcUData* udata = as_udata(val);
        Val val;
        if (get_map(&udata->metastruct->methods, str_name, &val)) {
          return true;
        }
        return false;
      }
      default:
        break;
    }
  }

  hby_err(h, "Expected instance or udata for call to 'hby_get_prop'");
  return false;
}

void hby_get_prop(hby_State* h, const char* name, int index) {
  Val val = val_at(h, index);
  GcStr* str_name = copy_str(h, name, strlen(name));

  if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_inst: {
        GcInst* inst = as_inst(val);
        Val val;
        if (get_map(&inst->fields, str_name, &val)) {
          push(h, val);
          return;
        }

        if (get_map(&inst->_struct->methods, str_name, &val)) {
          push(h, val);
          return;
        }

        hby_err(h, "Undefined property '%s'", name);
        return;
      }
      case obj_udata: {
        GcUData* udata = as_udata(val);
        Val val;
        if (get_map(&udata->metastruct->methods, str_name, &val)) {
          push(h, val);
          return;
        }

        hby_err(h, "Undefined property '%s'", name);
        return;
      }
      default:
        break;
    }
  }

  hby_err(h, "Expected instance or udata for call to 'hby_get_prop'");
}


double hby_get_num(hby_State* h, int index) {
  hby_expect_num(h, index);
  Val val = val_at(h, index);
  return as_num(val);
}

bool hby_get_bool(hby_State* h, int index) {
  Val val = val_at(h, index);
  return !(is_null(val) || (is_bool(val) && !as_bool(val)));
}

hby_CFn hby_get_cfunction(hby_State* h, int index) {
  hby_expect_cfunction(h, index);
  Val val = val_at(h, index);
  return as_c_fn(val)->fn;
}

const char* hby_get_string(hby_State* h, int index, size_t* len_out) {
  hby_expect_str(h, index);
  Val val = val_at(h, index);

  GcStr* str = as_str(val);
  if (len_out != NULL) {
    *len_out = str->len;
  }

  return str->chars;
}

const char* hby_get_tostring(hby_State* h, int index, size_t* len_out) {
  GcStr* str = obj_to_string(h, index);
  if (len_out != NULL) {
    *len_out = str->len;
  }

  return str->chars;
}

hby_api void* hby_get_udata(hby_State* h, int udata) {
  hby_expect_udata(h, udata);
  return as_udata(val_at(h, udata))->data;
}


hby_api double hby_opt_num(hby_State* h, int index, double opt) {
  if (hby_is_null(h, index)) {
    return opt;
  }
  return hby_get_num(h, index);
}

hby_api bool hby_opt_bool(hby_State* h, int index, bool opt) {
  if (hby_is_null(h, index)) {
    return opt;
  }
  return hby_get_num(h, index);
}

hby_api hby_CFn hby_opt_cfunction(hby_State* h, int index, hby_CFn opt) {
  if (hby_is_null(h, index)) {
    return opt;
  }
  return hby_get_cfunction(h, index);
}

hby_api const char* hby_opt_string(
    hby_State* h, int index, size_t* len_out, const char* opt) {
  if (hby_is_null(h, index)) {
    return opt;
  }
  return hby_get_string(h, index, len_out);
}


void hby_push_num(hby_State* h, double num) {
  push(h, create_num(num));
}

void hby_push_bool(hby_State* h, bool b) {
  push(h, create_bool(b));
}

void hby_push_null(hby_State* h) {
  push(h, create_null());
}

void hby_push_struct(hby_State* h, const char* name) {
  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));
  GcStruct* s = create_struct(h, str_name);
  pop(h);
  push(h, create_obj(s));
}

void hby_push_enum(hby_State* h, const char* name) {
  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));
  GcEnum* s = create_enum(h, str_name);
  pop(h);
  push(h, create_obj(s));
}

void hby_push_cfunction(hby_State* h, const char* name, hby_CFn fn, int argc) {
  GcStr* s = copy_str(h, name, strlen(name));
  push(h, create_obj(s));
  GcCFn* cfn = create_c_fn(h, s, fn, argc);
  pop(h);
  push(h, create_obj(cfn));
}

void hby_push_lstrcpy(hby_State* h, const char* chars, size_t len) {
  push(h, create_obj(copy_str(h, chars, len)));
}

void hby_push_lstr(hby_State* h, char* chars, size_t len) {
  push(h, create_obj(take_str(h, chars, len)));
}

void hby_push_strcpy(hby_State* h, const char* chars) {
  hby_push_lstrcpy(h, chars, strlen(chars));
}

void hby_push_str(hby_State* h, char* chars) {
  hby_push_lstr(h, chars, strlen(chars));
}

void* hby_push_udata(hby_State* h, size_t size) {
  GcUData* udata = create_udata(h, size);
  push(h, create_obj(udata));
  return udata->data;
}

void hby_push_array(hby_State* h) {
  push(h, create_obj(create_arr(h)));
}

void hby_instance(hby_State* h, int _struct) {
  hby_expect_struct(h, _struct);
  push(h, create_obj(create_inst(h, as_struct(val_at(h, _struct)))));
}


const char* hby_typestr(hby_ValueType type, size_t* len_out) {
  size_t dummy;
  if (len_out == NULL) {
    len_out = &dummy;
  }

  switch (type) {
    case hby_type_number:
      *len_out = 6;
      return "number";
    case hby_type_bool:
      *len_out = 7;
      return "boolean";
    case hby_type_null:
      *len_out = 5;
      return "null";
    case hby_type_string:
      *len_out = 6;
      return "string";
    case hby_type_instance:
      *len_out = 8;
      return "instance";
    case hby_type_struct:
      *len_out = 7;
      return "struct";
    case hby_type_enum:
      *len_out = 5;
      return "enum";
    case hby_type_function:
      *len_out = 8;
      return "function";
    case hby_type_cfunction:
      *len_out = 10;
      return "cfunction";
    case hby_type_udata:
      *len_out = 5;
      return "udata";
    case hby_type_array:
      *len_out = 5;
      return "array";
    default: break;
  }

  *len_out = 7;
  return "unknown";
}

void hby_tostr(hby_State* h, int index) {
  push(h, create_obj(obj_to_string(h, index)));
}

int hby_len(hby_State* h, int index) {
  Val val = val_at(h, index);
  switch(hby_get_type(h, index)) {
    case hby_type_array:
      return as_arr(val)->varr.len;
    case hby_type_string:
      return as_str(val)->len;
    default:
      return 0;
  }
}

void hby_concat(hby_State* h) {
  vm_concat(h);
}

void hby_push_typestruct(hby_State* h, int index) {
  push(h, create_obj(vm_get_typestruct(h, val_at(h, index))));
}


void hby_call(hby_State* h, int argc) {
  vm_call(h, h->top[-1 - argc], argc);
}

void hby_callon(hby_State* h, const char* mname, int argc) {
  vm_invoke(h, copy_str(h, mname, strlen(mname)), argc);
}

void hby_open_lib(hby_State* h, hby_CFn fn) {
  hby_push_cfunction(h, "open_lib", fn, 0);
  hby_call(h, 0);
  hby_pop(h, 1);
}


void hby_struct_add_const(hby_State* h, const char* name, int _struct) {
  hby_expect_struct(h, _struct);

  GcStruct* s = as_struct(val_at(h, _struct));
  Val val = val_at(h, -1);
  GcStr* str_name = get_name_or(h, val, name);
  push(h, create_obj(str_name));
  set_map(h, &s->staticm, str_name, val);
  pop(h); // str_name
  pop(h); // constant
}

void hby_struct_get_const(hby_State* h, const char* name) {
  hby_expect_struct(h, -1);

  GcStruct* s = as_struct(val_at(h, -1));

  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));

  Val val;
  get_map(&s->staticm, str_name, &val);

  pop(h); // str_name
  push(h, val);
}

void hby_struct_add_member(hby_State* h, hby_MethodType type, int _struct) {
  hby_expect_struct(h, _struct);
  hby_expect_cfunction(h, -1);

  GcStruct* s = as_struct(val_at(h, _struct));
  GcCFn* cfn = as_c_fn(val_at(h, -1));
  
  switch (type) {
    case hby_static_fn: {
      set_map(h, &s->staticm, cfn->name, create_obj(cfn));
      break;
    }
    case hby_method: {
      set_map(h, &s->methods, cfn->name, create_obj(cfn));
      break;
    }
  }

  pop(h); // c fn
}

void hby_struct_add_members(hby_State* h, hby_StructMethod* members, int _struct) {
  // The struct's index will be offset by the c function being on the stack
  if (_struct < 0) {
    _struct--;
  }

  for (hby_StructMethod* method = members; method->name != NULL; method++) {
    hby_push_cfunction(h, method->name, method->fn, method->argc);
    hby_struct_add_member(h, method->mtype, _struct);
  }
}

hby_api void hby_udata_set_metastruct(hby_State* h, int udata) {
  hby_expect_udata(h, udata);
  hby_expect_struct(h, -1);

  GcUData* u = as_udata(val_at(h, udata));
  u->metastruct = as_struct(pop(h));
}

hby_api void hby_udata_set_finalizer(hby_State* h, hby_CFn fn) {
  hby_expect_udata(h, -1);
  GcUData* u = as_udata(val_at(h, -1));

  hby_push_cfunction(h, "gc", fn, 1);
  u->finalizer = as_c_fn(val_at(h, -1));
  pop(h);
}

void hby_array_add(hby_State* h, int array) {
  hby_expect_array(h, array);
  GcArr* arr = as_arr(val_at(h, array));
  push_varr(h, &arr->varr, val_at(h, -1));
  pop(h);
}

void hby_array_index(hby_State* h, int array, int index) {
  hby_expect_array(h, array);
  GcArr* arr = as_arr(val_at(h, array));
  push(h, arr->varr.items[index]);
}

void hby_add_enum(hby_State* h, const char* name, int _enum) {
  hby_expect_enum(h, _enum);

  GcEnum* e = as_enum(val_at(h, _enum));
  GcStr* sname = copy_str(h, name, strlen(name));
  push(h, create_obj(sname));
  int i = e->vals.count;
  if (!set_map(h, &e->vals, sname, create_num(i))) {
    hby_err(h, err_msg_shadow_prev_enum, name);
  }
  pop(h); // sname
}
