#ifndef __HBS_H
#define __HBS_H

#include <stdlib.h>
#include <stdbool.h>

#define hbs_version_major 0
#define hbs_version_minor 1
#define hbs_version_patch 0

typedef struct hbs_State hbs_State;
typedef bool (*hbs_CFn)(hbs_State* h, int argc);

typedef enum {
  hbs_result_ok,
  hbs_result_runtime_err,
  hbs_result_compile_err,
} hbs_InterpretResult;

typedef enum {
  hbs_type_number,
  hbs_type_bool,
  hbs_type_null,
  hbs_type_string,
  hbs_type_instance,
  hbs_type_struct,
  hbs_type_function,
  hbs_type_c_function,
} hbs_ValueType;

typedef enum {
  hbs_static_fn,   // Static method
  hbs_method,      // Normal class method
} hbs_MethodType;

typedef struct {
  const char* name;
  hbs_CFn fn;
  int argc;
  hbs_MethodType mtype;
} hbs_StructMethod;

hbs_State* create_state();
void free_state(hbs_State* h);

hbs_InterpretResult hbs_run(hbs_State* h, const char* path);

void hbs_err(hbs_State* h, const char* fmt, ...);
void hbs_pop(hbs_State* h, int c);
void hbs_push(hbs_State* h, int stack_pos);
void hbs_set_global(hbs_State* h, const char* name);
hbs_ValueType hbs_get_type(hbs_State* h, int stack_pos);

void hbs_call(hbs_State* h, int argc);
bool hbs_has_prop(hbs_State* h, const char* name, int stack_pos);
void hbs_get_prop(hbs_State* h, const char* name, int stack_pos);
void hbs_method_call(hbs_State* h, const char* mname, int argc);

void hbs_open_lib(hbs_State* h, hbs_CFn fn);

double hbs_get_num(hbs_State* h, int stack_pos);
bool hbs_is_num(hbs_State* h, int stack_pos);
void hbs_push_num(hbs_State* h, double num);

bool hbs_get_bool(hbs_State* h, int stack_pos);
bool hbs_is_bool(hbs_State* h, int stack_pos);
void hbs_push_bool(hbs_State* h, bool b);

void hbs_push_null(hbs_State* h);

hbs_CFn hbs_get_c_fn(hbs_State* h, int stack_pos);
void hbs_push_c_fn(hbs_State* h, const char* name, hbs_CFn fn, int argc);

void hbs_to_str(hbs_State* h, int stack_pos);
const char* hbs_get_str(hbs_State* h, int stack_pos, size_t* len_out);
const char* hbs_get_and_to_str(hbs_State* h, int stack_pos, size_t* len_out);
bool hbs_is_str(hbs_State* h, int stack_pos);
void hbs_push_str_copy(hbs_State* h, const char* chars, size_t len);
void hbs_push_str(hbs_State* h, char* chars, size_t len);

bool hbs_is_inst(hbs_State* h, int stack_pos);

void hbs_define_struct(hbs_State* h, const char* name);
void hbs_add_static_const(hbs_State* h, const char* name, int _struct);
void hbs_add_member(hbs_State* h, hbs_MethodType type, int _struct);
void hbs_add_members(hbs_State* h, hbs_StructMethod* members, int _struct);

#endif // __HBS_H
