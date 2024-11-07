#ifndef __HBY_H
#define __HBY_H

#include <stdlib.h>
#include <stdbool.h>

#define hby_api

#define hby_version_major 0
#define hby_version_minor 1
#define hby_version_patch 0

#define hby_version "0.1.0"

typedef struct hby_State hby_State;
typedef bool (*hby_CFn)(hby_State* h, int argc);

// Result of running hobbyscript
typedef enum {
  hby_res_ok = 0,
  hby_res_runtime_err,
  hby_res_compile_err,
} hby_Res;

// Represents the various types in Hobbyscript
typedef enum {
  hby_type_number,
  hby_type_bool,
  hby_type_null,
  hby_type_str,
  hby_type_instance,
  hby_type_struct,
  hby_type_enum,
  hby_type_function,
  hby_type_cfunction,
  hby_type_array,
  hby_type_map,
  hby_type_udata,
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

// Create a Hobbyscript state
hby_api hby_State* create_state();
// Free a Hobbyscript state
hby_api void free_state(hby_State* h);

// Set the CLI arguments
hby_api void hby_cli_args(hby_State* h, int argc, const char** args);
// Compile a file
hby_api int hby_compile_file(hby_State* h, const char* file_path);
// Compile a string
hby_api int hby_compile(hby_State* h, const char* name, const char* src);

// Throw an error
hby_api void hby_err(hby_State* h, const char* fmt, ...);
// Throw an error if the type at `index` does not match `expect`
hby_api void hby_expect_type(hby_State* h, int index, hby_ValueType expect);
// Pop `c` values off the stack
hby_api void hby_pop(hby_State* h, int c);
// Push the value at `index` to the top of the stack
hby_api void hby_push(hby_State* h, int index);
// Set the global variable `name` to the value at `index`
// Passing NULL to `name` will try to infer the name, if possible
hby_api void hby_set_global(hby_State* h, const char* name, int index);
// Push the global variable `name` to the top of the stack
// Returns false if the variable does not exist
hby_api bool hby_get_global(hby_State* h, const char* name);
// Get the type of the value at `index`
hby_api hby_ValueType hby_get_type(hby_State* h, int index);

// Check if the value at `index` has the property `name`
hby_api bool hby_has_prop(hby_State* h, const char* name, int index);
// Push the property `name` from the value at `index` onto the top of the stack
hby_api bool hby_get_prop(hby_State* h, const char* name, int index);

// Get a number at `index`
hby_api double hby_get_num(hby_State* h, int index);
// Get a boolean at `index`
hby_api bool hby_get_bool(hby_State* h, int index);
// Get a C function at `index`
hby_api hby_CFn hby_get_cfunc(hby_State* h, int index);
// Get a string at `index`, put the length into `len_out`
hby_api const char* hby_get_str(hby_State* h, int index, size_t* len_out);
// Convert the value at `index` to a string, put the length into `len_out`
hby_api const char* hby_get_tostr(hby_State* h, int index, size_t* len_out);
// Get the userdata at `index`
hby_api void* hby_get_udata(hby_State* h, int index);

// Get a number at `index`, or `opt` if it's null
hby_api double hby_opt_num(hby_State* h, int index, double opt);
// Get a boolean at `index`, or `opt` if it's null
hby_api bool hby_opt_bool(hby_State* h, int index, bool opt);
// Get a C function at `index`, or `opt` if it's null
hby_api hby_CFn hby_opt_cfunc(hby_State* h, int index, hby_CFn opt);
// Get a string at `index`, or `opt` if it's null
hby_api const char* hby_opt_str(
  hby_State* h, int index, size_t* len_out, const char* opt);

// Push the number `num` to the top of the stack
hby_api void hby_push_num(hby_State* h, double num);
// Push the boolean `b` to the top of the stack
hby_api void hby_push_bool(hby_State* h, bool b);
// Push null to the top of the stack
hby_api void hby_push_null(hby_State* h);
// Push a struct to the top of the stack
hby_api void hby_push_struct(hby_State* h, const char* name);
// Push an enum to the top of the stack
hby_api void hby_push_enum(hby_State* h, const char* name);
// Push a C function to the top of the stack
hby_api void hby_push_cfunc(hby_State* h, const char* name, hby_CFn fn, int argc);
// Copy and push a string to the top of the stack
hby_api void hby_push_lstrcpy(hby_State* h, const char* chars, size_t len);
// Take ownership of, and push a string to the top of the stack
hby_api void hby_push_lstr(hby_State* h, char* chars, size_t len);
// Copy and push a string to the top of the stack
hby_api void hby_push_strcpy(hby_State* h, const char* chars);
// Take ownership of, and push a string to the top of the stack
hby_api void hby_push_str(hby_State* h, char* chars);
// Push a userdata with the size of `size` to the top of the stack
hby_api void* hby_push_udata(hby_State* h, size_t size);
// Push an array to the top of the stack
hby_api void hby_push_array(hby_State* h);
// Create an instance from the struct at `index`
hby_api void hby_instance(hby_State* h, int index);

// Get the type of the value at the top of the stack in a string form
hby_api const char* hby_get_type_name(hby_ValueType type, size_t* len_out);
// Convert a value to a string and push it to the top of the stack
hby_api void hby_tostr(hby_State* h, int index);
// Concatenate the top two values on the stack
hby_api void hby_concat(hby_State* h);
// Get the length of the value at `index`
hby_api int hby_len(hby_State* h, int index);
// Push the struct representing a builtin type on top of the stack
hby_api void hby_push_typestruct(hby_State* h, int index);

// Call a function
// Stack must look like: [func] [args]
hby_api void hby_call(hby_State* h, int argc);
// Call a protected function
hby_api bool hby_pcall(hby_State* h, int argc);
// Call a protected C function
hby_api void hby_pccall(hby_State* h, hby_CFn fn, int argc); // TODO
// Call a method on a value
hby_api void hby_callon(hby_State* h, const char* mname, int argc);
// Open a library
hby_api bool hby_open_lib(hby_State* h, hby_CFn fn);

// Add a static constant to a struct
hby_api void hby_struct_add_const(hby_State* h, const char* name, int _struct);
// Push a static constant from a struct to the top of the stack
hby_api void hby_struct_get_const(hby_State* h, const char* name);
// Add a method to a struct
hby_api void hby_struct_add_member(hby_State* h, hby_MethodType type, int _struct);
// Add multiple methods to a struct
hby_api void hby_struct_add_members(hby_State* h, hby_StructMethod* members, int index);

// Set the metastruct on the top of the stack to a userdata at `index`
hby_api void hby_udata_set_metastruct(hby_State* h, int index);
// Set a finalizer for userdata
hby_api void hby_udata_set_finalizer(hby_State* h, hby_CFn fn);

// Add an element at the top of the stack to an array at `index`
hby_api void hby_array_add(hby_State* h, int index);
// Push the value at `index` in the array at `array_index` to the top of the stack
hby_api void hby_array_get_index(hby_State* h, int array_index, int index);

// Add a value to an enum at `index`
hby_api void hby_add_enum(hby_State* h, const char* name, int index);

#define hby_is_num(h, index)          (hby_get_type(h, index) == hby_type_number)
#define hby_is_bool(h, index)         (hby_get_type(h, index) == hby_type_bool)
#define hby_is_null(h, index)         (hby_get_type(h, index) == hby_type_null)
#define hby_is_str(h, index)          (hby_get_type(h, index) == hby_type_str)
#define hby_is_instance(h, index)     (hby_get_type(h, index) == hby_type_instance)
#define hby_is_struct(h, index)       (hby_get_type(h, index) == hby_type_struct)
#define hby_is_enum(h, index)         (hby_get_type(h, index) == hby_type_enum)
#define hby_is_function(h, index)     (hby_get_type(h, index) == hby_type_function)
#define hby_is_cfunction(h, index)    (hby_get_type(h, index) == hby_type_cfunction)
#define hby_is_udata(h, index)        (hby_get_type(h, index) == hby_type_udata)
#define hby_is_array(h, index)        (hby_get_type(h, index) == hby_type_array)
#define hby_is_map(h, index)          (hby_get_type(h, index) == hby_type_map)

#define hby_expect_num(h, index)       hby_expect_type(h, index, hby_type_number);
#define hby_expect_bool(h, index)      hby_expect_type(h, index, hby_type_bool);
#define hby_expect_null(h, index)      hby_expect_type(h, index, hby_type_null);
#define hby_expect_str(h, index)       hby_expect_type(h, index, hby_type_str);
#define hby_expect_instance(h, index)  hby_expect_type(h, index, hby_type_instance);
#define hby_expect_struct(h, index)    hby_expect_type(h, index, hby_type_struct);
#define hby_expect_enum(h, index)      hby_expect_type(h, index, hby_type_enum);
#define hby_expect_function(h, index)  hby_expect_type(h, index, hby_type_function);
#define hby_expect_cfunction(h, index) hby_expect_type(h, index, hby_type_cfunction);
#define hby_expect_udata(h, index)     hby_expect_type(h, index, hby_type_udata);
#define hby_expect_array(h, index)     hby_expect_type(h, index, hby_type_array);
#define hby_expect_map(h, index)       hby_expect_type(h, index, hby_type_map);

#endif // __HBY_H
