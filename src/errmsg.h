#ifndef __HBY_ERRMSG_H
#define __HBY_ERRMSG_H

#define err_msg_out_mem "not enough memory"

// Vm errors
#define err_msg_stack_overflow "stack overflow"
#define err_msg_shadow_prev_enum "enum value '%s' is already defined"
#define err_msg_bad_argc "expected %d args, but %d were passed"
#define err_msg_bad_callee "cannot call that value"
#define err_msg_bad_static_access \
  "cannot access static properties on that value"
#define err_msg_bad_prop_access "cannot access properties on that value"
#define err_msg_bad_destruct_val "cannot destruct that value"
#define err_msg_bad_destruct_len "variable count and array length mismatch"
#define err_msg_bad_sub_get "subscript access is not supported for that value"
#define err_msg_bad_sub_set \
  "subscript assignment is not supported for that value"
#define err_msg_bad_inst "cannot instance that value"
#define err_msg_undef_prop "undefined property '%s'"
#define err_msg_undef_map_key "key '%s' is not defined on map"
#define err_msg_undef_static_prop "undefined static property '%s'"
#define err_msg_undef_var "undefined variable '%s'"
#define err_msg_index_out_of_bounds "index out of bounds"
#define err_msg_expected_char "Expected char (string with a length of 1)"
#define err_msg_bad_operands(type) "operands must be " type
#define err_msg_bad_operand(type) "operand must be a " type

// Compile errors
#define err_msg_invalid_char "unexpected character"
#define err_msg_escape_str "invalid escape code"
#define err_msg_eof_str "unterminated string"
#define err_msg_self_read "cannot use a local variable in its own definition"
#define err_msg_shadow "variable shadows another variable in a higher scope"
#define err_msg_use_break "cannot use 'break' outside of a loop"
#define err_msg_use_continue "cannot use 'continue' outside of a loop"
#define err_msg_assign_const "cannot assign to a constant variable"
#define err_msg_global_const "constants cannot be global"
#define err_msg_bad_global "expected global declaration"
#define err_msg_bad_loop_label "could not resolve loop label"
#define err_msg_bad_self "cannot use 'self' outside of a struct"
#define err_msg_bad_assign "invalid assignment target"
#define err_msg_bad_static_member \
  "expected struct or function declaration after 'static'"
#define err_msg_bad_static_struct "expected struct declaration after 'static'"
#define err_msg_bad_struct_member "structs can only contain declarations"
#define err_msg_bad_struct_stat "struct statement must be on line #1"
#define err_msg_bad_decl_scope(type) \
  type " declaration must not be declared in a scope"
#define err_msg_bad_else_case "'else' case must be the last case in a switch"
#define err_msg_max_consts "too many constants in one chunk"
#define err_msg_max_locals "too many local variables in one chunk"
#define err_msg_max_upvals "too many upvalues in one function"
#define err_msg_max_breaks "too many break statements in one loop"
#define err_msg_max_cases "too many cases in one switch"
#define err_msg_max_enum "too many values in one enum"
#define err_msg_max_strfmt "too many nested formatted strings"
#define err_msg_max_destruct "too many variables to destruct"
#define err_msg_max_args "too many arguments"
#define err_msg_max_param "too many parameters"
#define err_msg_big_loop "loop body too large"
#define err_msg_big_jmp "jump body too large"
#define err_msg_expect_ident "expected an identifier"
#define err_msg_expect_expr "expected an expression"
#define err_msg_expect_eof "expected eof"
#define err_msg_expect_fn_body "expected '{' or '->'"
#define err_msg_expect(expected) "expected '" expected "'"

#endif // __HBY_ERRMSG_H
