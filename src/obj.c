#include "obj.h"

#include <stdio.h>
#include <string.h>
#include "tostr.h"
#include "val.h"
#include "mem.h"
#include "map.h"

#define alloc_obj(h, ctype, hbstype) (ctype*)alloc_obj_impl(h, sizeof(ctype), hbstype)

static GcObj* alloc_obj_impl(hbs_State* h, size_t size, ObjType type) {
  GcObj* obj = (GcObj*)reallocate(h, NULL, 0, size);
  obj->type = type;
  obj->marked = false;

  obj->next = h->objs;
  h->objs = obj;

#ifdef hbs_log_gc
  printf("%p allocated %zu for type %d\n", (void*)obj, size, type);
#endif

  return obj;
}

GcMethod* create_method(hbs_State* h, Val owner, GcClosure* fn) {
  GcMethod* method = alloc_obj(h, GcMethod, obj_method);
  method->owner = owner;
  method->fn = fn;
  return method;
}

GcInst* create_inst(hbs_State* h, GcStruct* s) {
  GcInst* inst = alloc_obj(h, GcInst, obj_inst);
  inst->_struct = s;
  init_map(&inst->fields);
  copy_map(h, &s->members, &inst->fields);
  return inst;
}

GcStruct* create_struct(hbs_State* h, GcStr* name) {
  GcStruct* s = alloc_obj(h, GcStruct, obj_struct);
  s->name = name;
  init_map(&s->staticm);
  init_map(&s->methods);
  init_map(&s->members);
  return s;
}

GcUpval* create_upval(hbs_State* h, Val* loc) {
  GcUpval* upval = alloc_obj(h, GcUpval, obj_upval);
  upval->loc = loc;
  upval->closed = create_null();
  upval->next = NULL;
  return upval;
}

GcClosure* create_closure(hbs_State* h, GcFn* fn) {
  GcUpval** upvals = allocate(h, GcUpval*, fn->upvalc);
  for (int i = 0; i < fn->upvalc; i++) {
    upvals[i] = NULL;
  }

  GcClosure* closure = alloc_obj(h, GcClosure, obj_closure);
  closure->fn = fn;
  closure->upvals = upvals;
  closure->upvalc = fn->upvalc;
  return closure;
}

GcCFn* create_c_fn(hbs_State* h, GcStr* name, hbs_CFn fn, int arity) {
  GcCFn* c_fn = alloc_obj(h, GcCFn, obj_c_fn);
  c_fn->arity = arity;
  c_fn->name = name;
  c_fn->fn = fn;
  return c_fn;
}

GcFn* create_fn(hbs_State* h, GcStr* file_path) {
  GcFn* fn = alloc_obj(h, GcFn, obj_fn);
  fn->arity = 0;
  fn->name = NULL;
  fn->path = file_path;
  fn->upvalc = 0;
  init_chunk(&fn->chunk);
  return fn;
}

GcEnum* create_enum(hbs_State* h, GcStr* name) {
  GcEnum* _enum = alloc_obj(h, GcEnum, obj_enum);
  _enum->name = name;
  init_map(&_enum->vals);
  return _enum;
}

GcArr* create_arr(hbs_State* h) {
  GcArr* arr = alloc_obj(h, GcArr, obj_arr);
  init_valarr(&arr->arr);
  return arr;
}

static GcStr* alloc_str(hbs_State* h, char* chars, int len, u32 hash) {
  GcStr* str = alloc_obj(h, GcStr, obj_str);
  str->len = len;
  str->chars = chars;
  str->hash = hash;

  push(h, create_obj(str));
  set_map(h, &h->strs, str, create_null());
  pop(h);
  return str;
}

static uint32_t hash_str(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

GcStr* copy_str(hbs_State* h, const char* chars, int len) {
  u32 hash = hash_str(chars, len);
  GcStr* interned = find_str_map(&h->strs, chars, len, hash);
  if (interned != NULL) {
    return interned;
  }

  char* copy = allocate(h, char, len + 1);
  memcpy(copy, chars, len);
  copy[len] = '\0';
  return alloc_str(h, copy, len, hash);
}

GcStr* take_str(hbs_State* h, char* chars, int len) {
  u32 hash = hash_str(chars, len);
  GcStr* interned = find_str_map(&h->strs, chars, len, hash);
  if (interned != NULL) {
    free_arr(h, char, chars, len + 1);
    return interned;
  }
  return alloc_str(h, chars, len, hash);
}
