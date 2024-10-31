#ifndef __HBY_OBJ_H
#define __HBY_OBJ_H

#include "common.h"
#include "chunk.h"
#include "state.h"
#include "val.h"
#include "arr.h"

typedef enum {
  obj_str,
  obj_fn,
  obj_closure,
  obj_c_fn,
  obj_upval,
  obj_struct,
  obj_inst,
  obj_method,
  obj_enum,
  obj_arr,
} ObjType;

struct GcObj {
  ObjType type;
  bool marked; // Marked by the GC as reachable?
  struct GcObj* next; // Linked list kept for GCing objects
};

typedef struct GcUpval {
  GcObj obj; // Object header
  // Location of the upvalue. The value is on the stack or inside this struct
  Val* loc; 
  Val closed; // When the upvalue is popped off the stack, it is moved here
  struct GcUpval* next; // Next upvalue. Only used when it is on the stack
} GcUpval;

typedef struct GcFn {
  GcObj obj; // Object header
  int arity; // Number of arguments
  Chunk chunk; // Code
  GcStr* name; // Name of this function
  GcStr* path; // The path to the file that contains this function
  int upvalc; // Number of captured upvalues
} GcFn;

typedef struct GcClosure {
  GcObj obj; // Object header
  GcFn* fn; // Wrapped object
  GcUpval** upvals; // Captured upvalues
  int upvalc;
} GcClosure;

typedef struct GcCFn {
  GcObj obj; // Object header
  int arity; // Number of arguments. -1 means it's varadic
  hby_CFn fn; // The function ptr
  GcStr* name; // Name of the function. Only used for stack traces and printing
} GcCFn;

typedef struct {
  GcObj obj; // Object header
  GcStr* name; // Struct name
  
  Map vals;
} GcEnum;

typedef struct GcStruct {
  GcObj obj; // Object header
  GcStr* name; // Struct name

  Map staticm; // Static methods
  Map members; // Variables defined by the struct. Copied down to instances
  Map methods; // Methods defined by the struct. Referenced by instances
} GcStruct;

typedef struct GcArr {
  GcObj obj;
  VArr varr;
} GcArr;

typedef struct {
  GcObj obj; // Object header
  GcStruct* _struct; // The struct this instance is a child of
  Map fields; // Struct members
} GcInst;

// TODO: Allow this to work with C functions too
typedef struct {
  GcObj obj; // Object header
  Val owner; // The owner of this function
  union {
    GcClosure* hby;
    GcCFn* c;
  } fn; // The wrapped function
} GcMethod;

struct GcStr {
  GcObj obj; // Object header
  int len; // Length of the string
  char* chars; // String data
  u32 hash; // String hash
};

static inline bool obj_of_type(Val val, ObjType type) {
  return is_obj(val) && as_obj(val)->type == type;
}

#define obj_type(v) (as_obj(v)->type)
#define method_type(m) (m->fn.hby->obj.type)

#define is_struct(v)  obj_of_type(v, obj_struct)
#define is_method(v)  obj_of_type(v, obj_method)
#define is_inst(v)    obj_of_type(v, obj_inst)
#define is_closure(v) obj_of_type(v, obj_closure)
#define is_upval(v)   obj_of_type(v, obj_upval)
#define is_c_fn(v)    obj_of_type(v, obj_c_fn)
#define is_fn(v)      obj_of_type(v, obj_fn)
#define is_enum(v)    obj_of_type(v, obj_enum)
#define is_arr(v)     obj_of_type(v, obj_arr)
#define is_str(v)     obj_of_type(v, obj_str)

#define as_struct(v)  ((GcStruct*)as_obj(v))
#define as_method(v)  ((GcMethod*)as_obj(v))
#define as_inst(v)    ((GcInst*)as_obj(v))
#define as_closure(v) ((GcClosure*)as_obj(v))
#define as_upval(v)   ((GcUpval*)as_obj(v))
#define as_c_fn(v)    ((GcCFn*)as_obj(v))
#define as_fn(v)      ((GcFn*)as_obj(v))
#define as_enum(v)    ((GcEnum*)as_obj(v))
#define as_arr(v)     ((GcArr*)as_obj(v))
#define as_str(v)     ((GcStr*)as_obj(v))
#define as_cstr(v)    (as_str(v)->chars)

GcStruct* create_struct(hby_State* h, GcStr* name);
GcMethod* create_method(hby_State* h, Val owner, GcClosure* fn);
GcMethod* create_c_method(hby_State* h, Val owner, GcCFn* fn);
GcInst* create_inst(hby_State* h, GcStruct* s);
GcUpval* create_upval(hby_State* h, Val* loc);
GcCFn* create_c_fn(hby_State* h, GcStr* name, hby_CFn fn, int arity);
GcFn* create_fn(hby_State* h, GcStr* file_path);
GcClosure* create_closure(hby_State* h, GcFn* fn);
GcEnum* create_enum(hby_State* h, GcStr* name);
GcArr* create_arr(hby_State* h);
GcStr* copy_str(hby_State* h, const char* chars, int len);
GcStr* take_str(hby_State* h, char* chars, int len);

#endif // __HBY_OBJ_H
