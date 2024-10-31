#ifndef __HBY_H
#define __HBY_H

#include <stdlib.h>
#include <stdbool.h>

#define hby_api

#define hby_version_major 0
#define hby_version_minor 1
#define hby_version_patch 0

typedef struct hby_State hby_State;
typedef bool (*hby_CFn)(hby_State* h, int argc);

typedef enum {
  hby_result_ok,
  hby_result_runtime_err,
  hby_result_compile_err,
} hby_InterpretResult;

typedef enum {
  hby_type_number,
  hby_type_bool,
  hby_type_null,
  hby_type_string,
  hby_type_instance,
  hby_type_struct,
  hby_type_enum,
  hby_type_function,
  hby_type_cfunction,
  hby_type_array,
  hby_type_count, // How many types there are
} hby_ValueType;

typedef enum {
  hby_static_fn, // Static method
  hby_method,    // Normal class method
} hby_MethodType;

typedef struct {
  const char* name;
  hby_CFn fn;
  int argc;
  hby_MethodType mtype;
} hby_StructMethod;

typedef struct {
  const char* name;
  hby_CFn fn;
  int argc;
} hby_CFnArgs;

hby_api hby_State* create_state();
hby_api void free_state(hby_State* h);

hby_api void hby_cli_args(hby_State* h, int argc, const char** args);
hby_api hby_InterpretResult hby_run(hby_State* h, const char* path);
hby_api hby_InterpretResult hby_run_string(
    hby_State* h, const char* file_name, const char* str);

hby_api void hby_err(hby_State* h, const char* fmt, ...);
hby_api void hby_expect_type(hby_State* h, int index, hby_ValueType expect);
hby_api void hby_pop(hby_State* h, int c);
hby_api void hby_push(hby_State* h, int index);
hby_api void hby_set_global(hby_State* h, const char* name);
hby_api void hby_get_global(hby_State* h, const char* name);
hby_api hby_ValueType hby_get_type(hby_State* h, int index);

hby_api bool hby_has_prop(hby_State* h, const char* name, int index);
hby_api void hby_get_prop(hby_State* h, const char* name, int index);

hby_api double hby_get_num(hby_State* h, int index);
hby_api bool hby_get_bool(hby_State* h, int index);
hby_api hby_CFn hby_get_cfunction(hby_State* h, int index);
hby_api const char* hby_get_string(hby_State* h, int index, size_t* len_out);
hby_api const char* hby_get_tostring(hby_State* h, int index, size_t* len_out);

hby_api void hby_push_num(hby_State* h, double num);
hby_api void hby_push_bool(hby_State* h, bool b);
hby_api void hby_push_null(hby_State* h);
hby_api void hby_push_struct(hby_State* h, const char* name);
hby_api void hby_push_enum(hby_State* h, const char* name);
hby_api void hby_push_cfunction(hby_State* h, const char* name, hby_CFn fn, int argc);
hby_api void hby_push_string_copy(hby_State* h, const char* chars, size_t len);
hby_api void hby_push_string(hby_State* h, char* chars, size_t len);
hby_api void hby_create_array(hby_State* h);
hby_api void hby_instance(hby_State* h); // TODO

hby_api const char* hby_typestr(hby_ValueType type, size_t* len_out);
hby_api void hby_tostr(hby_State* h, int index);
hby_api int hby_len(hby_State* h, int index);

hby_api void hby_call(hby_State* h, int argc);
hby_api void hby_pcall(hby_State* h, int argc); // TODO
hby_api void hby_pccall(hby_State* h, hby_CFn fn, int argc); // TODO
hby_api void hby_callon(hby_State* h, const char* mname, int argc);
hby_api void hby_open_lib(hby_State* h, hby_CFn fn);

hby_api void hby_add_static_const(hby_State* h, const char* name, int _struct);
hby_api void hby_add_member(hby_State* h, hby_MethodType type, int _struct);
hby_api void hby_add_members(hby_State* h, hby_StructMethod* members, int index);

hby_api void hby_push_array(hby_State* h, int array);
hby_api void hby_get_array(hby_State* h, int array, int index);

hby_api void hby_add_enum(hby_State* h, const char* name, int _enum);

#define hby_is_num(h, index)          (hby_get_type(h, index) == hby_type_number)
#define hby_is_bool(h, index)         (hby_get_type(h, index) == hby_type_bool)
#define hby_is_null(h, index)         (hby_get_type(h, index) == hby_type_null)
#define hby_is_string(h, index)       (hby_get_type(h, index) == hby_type_string)
#define hby_is_instance(h, index)     (hby_get_type(h, index) == hby_type_instance)
#define hby_is_struct(h, index)       (hby_get_type(h, index) == hby_type_struct)
#define hby_is_enum(h, index)         (hby_get_type(h, index) == hby_type_enum)
#define hby_is_function(h, index)     (hby_get_type(h, index) == hby_type_function)
#define hby_is_cfunction(h, index)    (hby_get_type(h, index) == hby_type_cfunction)
#define hby_is_array(h, index)        (hby_get_type(h, index) == hby_type_array)

#define hby_expect_num(h, index)       hby_expect_type(h, index, hby_type_number);
#define hby_expect_bool(h, index)      hby_expect_type(h, index, hby_type_bool);
#define hby_expect_null(h, index)      hby_expect_type(h, index, hby_type_null);
#define hby_expect_string(h, index)    hby_expect_type(h, index, hby_type_string);
#define hby_expect_instance(h, index)  hby_expect_type(h, index, hby_type_instance);
#define hby_expect_struct(h, index)    hby_expect_type(h, index, hby_type_struct);
#define hby_expect_enum(h, index)      hby_expect_type(h, index, hby_type_enum);
#define hby_expect_function(h, index)  hby_expect_type(h, index, hby_type_function);
#define hby_expect_cfunction(h, index) hby_expect_type(h, index, hby_type_cfunction);
#define hby_expect_array(h, index)     hby_expect_type(h, index, hby_type_array);

#endif // __HBY_H
