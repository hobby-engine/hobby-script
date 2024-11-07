#include "errmsg.h"
#include "hby.h"

#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "state.h"
#include "parser.h"
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

static char* read_file(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file '%s'.\n", path);
    exit(4);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char* buf = (char*)malloc(file_size + 1);
  if (buf == NULL) {
    fprintf(stderr, "File '%s' too large to read.\n", path);
    exit(4);
  }

  size_t bytes_read = fread(buf, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file '%s'.", path);
    exit(4);
  }

  buf[bytes_read] = '\0';

  fclose(file);
  return buf;
}

void hby_expect_type(hby_State* h, int index, hby_ValueType expect) {
  hby_ValueType type = hby_get_type(h, index);
  if (type != expect) {
    hby_err(
      h, "expected '%s', got '%s'",
      hby_get_type_name(expect, NULL), hby_get_type_name(type, NULL));
  }
}

int hby_compile_file(hby_State* h, const char* file_path) {
  char* src = read_file(file_path);
  int errc = hby_compile(h, file_path, src);
  free(src);
  return errc;
}

int hby_compile(hby_State* h, const char* name, const char* src) {
  GcFn* fn = compile_hby(h, name, src);
  if (fn == NULL) {
    return h->parser->errc;
  }
  push(h, create_obj(fn));
  GcClosure* closure = create_closure(h, fn);
  pop(h);
  push(h, create_obj(closure));
  return 0;
}

void hby_pop(hby_State* h, int c) {
  h->top -= c;
}

void hby_push(hby_State* h, int index) {
  push(h, val_at(h, index));
}

void hby_set_global(hby_State* h, const char* name, int index) {
  Val val = val_at(h, index);

  GcStr* sname = get_name_or(h, val, name);
  push(h, create_obj(sname));
  set_table(h, &h->globals, sname, val);
  pop(h);
}

bool hby_get_global(hby_State* h, const char* name) {
  Val val;
  if (get_table(&h->globals, copy_str(h, name, strlen(name)), &val)) {
    push(h, val);
    return true;
  }
  return false;
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
      case obj_str: return hby_type_str;
      case obj_struct: return hby_type_struct;
      case obj_enum: return hby_type_enum;
      case obj_method:
      case obj_closure: return hby_type_function;
      case obj_c_fn: return hby_type_cfunction;
      case obj_inst: return hby_type_instance;
      case obj_udata: return hby_type_udata;
      case obj_arr: return hby_type_array;
      case obj_map: return hby_type_map;
      default: break;
    }
  }

  hby_err(h, "Internal value type which should not be accessible (C API)");
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
        if (get_table(&inst->fields, str_name, &val)) {
          return true;
        }
        if (get_table(&inst->_struct->methods, str_name, &val)) {
          return true;
        }
        return false;
      }
      case obj_udata: {
        GcUData* udata = as_udata(val);
        Val val;
        if (get_table(&udata->metastruct->methods, str_name, &val)) {
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

bool hby_get_prop(hby_State* h, const char* name, int index) {
  Val val = val_at(h, index);
  GcStr* str_name = copy_str(h, name, strlen(name));

  if (is_obj(val)) {
    switch (obj_type(val)) {
      case obj_inst: {
        GcInst* inst = as_inst(val);
        Val val;
        if (get_table(&inst->fields, str_name, &val)) {
          push(h, val);
          return true;
        }

        if (get_table(&inst->_struct->methods, str_name, &val)) {
          push(h, val);
          return true;
        }

        return false;
      }
      case obj_udata: {
        GcUData* udata = as_udata(val);
        Val val;
        if (get_table(&udata->metastruct->methods, str_name, &val)) {
          push(h, val);
          return true;
        }

        return false;
      }
      default:
        break;
    }
  }

  return false;
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

hby_CFn hby_get_cfunc(hby_State* h, int index) {
  hby_expect_cfunction(h, index);
  Val val = val_at(h, index);
  return as_c_fn(val)->fn;
}

const char* hby_get_str(hby_State* h, int index, size_t* len_out) {
  hby_expect_str(h, index);
  Val val = val_at(h, index);

  GcStr* str = as_str(val);
  if (len_out != NULL) {
    *len_out = str->len;
  }

  return str->chars;
}

const char* hby_get_tostr(hby_State* h, int index, size_t* len_out) {
  GcStr* str = obj_to_string(h, index);
  if (len_out != NULL) {
    *len_out = str->len;
  }

  return str->chars;
}

hby_api void* hby_get_udata(hby_State* h, int index) {
  hby_expect_udata(h, index);
  return as_udata(val_at(h, index))->data;
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

hby_api hby_CFn hby_opt_cfunc(hby_State* h, int index, hby_CFn opt) {
  if (hby_is_null(h, index)) {
    return opt;
  }
  return hby_get_cfunc(h, index);
}

hby_api const char* hby_opt_str(
    hby_State* h, int index, size_t* len_out, const char* opt) {
  if (hby_is_null(h, index)) {
    return opt;
  }
  return hby_get_str(h, index, len_out);
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

void hby_push_cfunc(hby_State* h, const char* name, hby_CFn fn, int argc) {
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

void hby_instance(hby_State* h, int index) {
  hby_expect_struct(h, index);
  push(h, create_obj(create_inst(h, as_struct(val_at(h, index)))));
}


const char* hby_get_type_name(hby_ValueType type, size_t* len_out) {
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
    case hby_type_str:
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
    case hby_type_str:
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

bool hby_pcall(hby_State* h, int argc) {
  return vm_pcall(h, h->top[-1 - argc], argc) == hby_res_ok;
}

void hby_callon(hby_State* h, const char* mname, int argc) {
  vm_invoke(h, copy_str(h, mname, strlen(mname)), argc);
}

void hby_open_lib(hby_State* h, hby_CFn fn) {
  hby_push_cfunc(h, "open_lib", fn, 0);
  hby_call(h, 0);
  hby_pop(h, 1);
}


void hby_struct_add_const(hby_State* h, const char* name, int index) {
  hby_expect_struct(h, index);

  GcStruct* s = as_struct(val_at(h, index));
  Val val = val_at(h, -1);
  GcStr* str_name = get_name_or(h, val, name);
  push(h, create_obj(str_name));
  set_table(h, &s->staticm, str_name, val);
  pop(h); // str_name
  pop(h); // constant
}

void hby_struct_get_const(hby_State* h, const char* name) {
  hby_expect_struct(h, -1);

  GcStruct* s = as_struct(val_at(h, -1));

  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));

  Val val;
  get_table(&s->staticm, str_name, &val);

  pop(h); // str_name
  push(h, val);
}

void hby_struct_add_member(hby_State* h, hby_MethodType type, int index) {
  hby_expect_struct(h, index);
  hby_expect_cfunction(h, -1);

  GcStruct* s = as_struct(val_at(h, index));
  GcCFn* cfn = as_c_fn(val_at(h, -1));
  
  switch (type) {
    case hby_static_fn: {
      set_table(h, &s->staticm, cfn->name, create_obj(cfn));
      break;
    }
    case hby_method: {
      set_table(h, &s->methods, cfn->name, create_obj(cfn));
      break;
    }
  }

  pop(h); // c fn
}

void hby_struct_add_members(hby_State* h, hby_StructMethod* members, int index) {
  // The struct's index will be offset by the c function being on the stack
  if (index < 0) {
    index--;
  }

  for (hby_StructMethod* method = members; method->name != NULL; method++) {
    hby_push_cfunc(h, method->name, method->fn, method->argc);
    hby_struct_add_member(h, method->mtype, index);
  }
}

void hby_udata_set_metastruct(hby_State* h, int index) {
  hby_expect_udata(h, index);
  hby_expect_struct(h, -1);

  GcUData* u = as_udata(val_at(h, index));
  u->metastruct = as_struct(pop(h));
}

void hby_udata_set_finalizer(hby_State* h, hby_CFn fn) {
  hby_expect_udata(h, -1);
  GcUData* u = as_udata(val_at(h, -1));

  hby_push_cfunc(h, "gc", fn, 1);
  u->finalizer = as_c_fn(val_at(h, -1));
  pop(h);
}

void hby_array_add(hby_State* h, int index) {
  hby_expect_array(h, index);
  GcArr* arr = as_arr(val_at(h, index));
  push_varr(h, &arr->varr, val_at(h, -1));
  pop(h);
}

void hby_array_get_index(hby_State* h, int array_index, int index) {
  hby_expect_array(h, array_index);
  GcArr* arr = as_arr(val_at(h, array_index));
  push(h, arr->varr.items[index]);
}

void hby_add_enum(hby_State* h, const char* name, int index) {
  hby_expect_enum(h, index);

  GcEnum* e = as_enum(val_at(h, index));
  GcStr* sname = copy_str(h, name, strlen(name));
  push(h, create_obj(sname));
  int i = e->vals.itemc;
  if (!set_table(h, &e->vals, sname, create_num(i))) {
    hby_err(h, err_msg_shadow_prev_enum, name);
  }
  pop(h); // sname
}
