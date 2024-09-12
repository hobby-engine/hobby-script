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
  hbs_type_cfunction,
  hbs_type_array,
} hbs_ValueType;

typedef enum {
  hbs_static_fn, // Static method
  hbs_method,    // Normal class method
} hbs_MethodType;

typedef struct {
  const char* name;
  hbs_CFn fn;
  int argc;
  hbs_MethodType mtype;
} hbs_StructMethod;

typedef struct {
  const char* name;
  hbs_CFn fn;
  int argc;
} hbs_CFnArgs;

hbs_State* create_state();
void free_state(hbs_State* h);

hbs_InterpretResult hbs_run(hbs_State* h, const char* path);

void hbs_err(hbs_State* h, const char* fmt, ...);
void hbs_expect_type(hbs_State* h, int index, hbs_ValueType expect);
void hbs_pop(hbs_State* h, int c);
void hbs_push(hbs_State* h, int index);
void hbs_set_global(hbs_State* h, const char* name);
void hbs_get_global(hbs_State* h, const char* name);
hbs_ValueType hbs_get_type(hbs_State* h, int index);

bool hbs_has_prop(hbs_State* h, const char* name, int index);
void hbs_get_prop(hbs_State* h, const char* name, int index);

double hbs_get_num(hbs_State* h, int index);
bool hbs_get_bool(hbs_State* h, int index);
hbs_CFn hbs_get_cfunction(hbs_State* h, int index);
const char* hbs_get_string(hbs_State* h, int index, size_t* len_out);
const char* hbs_get_tostring(hbs_State* h, int index, size_t* len_out);

void hbs_push_num(hbs_State* h, double num);
void hbs_push_bool(hbs_State* h, bool b);
void hbs_push_null(hbs_State* h);
void hbs_push_cfunction(hbs_State* h, const char* name, hbs_CFn fn, int argc);
void hbs_push_string_copy(hbs_State* h, const char* chars, size_t len);
void hbs_push_string(hbs_State* h, char* chars, size_t len);
void hbs_create_array(hbs_State* h); // TODO
void hbs_instance(hbs_State* h); // TODO

const char* hbs_typestr(hbs_ValueType type, size_t* len_out);
void hbs_tostr(hbs_State* h, int index);
void hbs_len(hbs_State* h, int index); // TODO

void hbs_call(hbs_State* h, int argc);
void hbs_pcall(hbs_State* h, int argc); // TODO
void hbs_pccall(hbs_State* h, hbs_CFn fn, int argc); // TODO
void hbs_callon(hbs_State* h, const char* mname, int argc);
void hbs_open_lib(hbs_State* h, hbs_CFn fn);

void hbs_push_struct(hbs_State* h, const char* name);
void hbs_add_static_const(hbs_State* h, const char* name, int _struct);
void hbs_add_member(hbs_State* h, hbs_MethodType type, int _struct);
void hbs_add_members(hbs_State* h, hbs_StructMethod* members, int index);


#define hbs_is_num(h, index)          (hbs_get_type(h, index) == hbs_type_number)
#define hbs_is_bool(h, index)         (hbs_get_type(h, index) == hbs_type_bool)
#define hbs_is_null(h, index)         (hbs_get_type(h, index) == hbs_type_null)
#define hbs_is_string(h, index)       (hbs_get_type(h, index) == hbs_type_string)
#define hbs_is_instance(h, index)     (hbs_get_type(h, index) == hbs_type_instance)
#define hbs_is_struct(h, index)       (hbs_get_type(h, index) == hbs_type_struct)
#define hbs_is_function(h, index)     (hbs_get_type(h, index) == hbs_type_function)
#define hbs_is_cfunction(h, index)    (hbs_get_type(h, index) == hbs_type_cfunction)
#define hbs_is_array(h, index)        (hbs_get_type(h, index) == hbs_type_array)

#define hbs_check_num(h, index)       hbs_expect_type(h, index, hbs_type_number);
#define hbs_check_bool(h, index)      hbs_expect_type(h, index, hbs_type_bool);
#define hbs_check_null(h, index)      hbs_expect_type(h, index, hbs_type_null);
#define hbs_check_string(h, index)    hbs_expect_type(h, index, hbs_type_string);
#define hbs_check_instance(h, index)  hbs_expect_type(h, index, hbs_type_instance);
#define hbs_check_struct(h, index)    hbs_expect_type(h, index, hbs_type_struct);
#define hbs_check_function(h, index)  hbs_expect_type(h, index, hbs_type_function);
#define hbs_check_cfunction(h, index) hbs_expect_type(h, index, hbs_type_cfunction);
#define hbs_check_array(h, index)     hbs_expect_type(h, index, hbs_type_array);

#endif // __HBS_H
