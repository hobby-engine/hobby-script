#ifndef __HBS_ERRMSG_H
#define __HBS_ERRMSG_H

#define err_msg_out_mem "Not enough memory"

// Vm errors
#define err_msg_stack_overflow "Stack overflow"
#define err_msg_shadow_prev_enum "Enum value '%s' is already defined"
#define err_msg_bad_argc "Expected %d args, but %d were passed"
#define err_msg_bad_callee "Cannot call that value"
#define err_msg_bad_static_access "Cannot access static properties on that value"
#define err_msg_bad_prop_access "Cannot access properties on that value"
#define err_msg_bad_sub_access "Cannot use subscript operator on that value"
#define err_msg_bad_inst "Cannot instance that value"
#define err_msg_undef_prop "Undefined property '%s'"
#define err_msg_undef_static_prop "Undefined static property '%s'"
#define err_msg_undef_var "Undefined variable '%s'"
#define err_msg_index_out_of_bounds "Index out of bounds"
#define err_msg_bad_operands(type) "Operands must be " type
#define err_msg_bad_operand(type) "Operand must be a " type

// Compile errors
#define err_msg_invalid_char "Unexpected character"
#define err_msg_escape_str "Invalid escape code"
#define err_msg_eof_str "Unterminated string"
#define err_msg_self_read "Cannot use a local variable in its own definition"
#define err_msg_shadow "Variable shadows another variable in a higher scope"
#define err_msg_use_break "Cannot use 'break' outside of a loop"
#define err_msg_use_continue "Cannot use 'continue' outside of a loop"
#define err_msg_bad_global "Expected global declaration"
#define err_msg_bad_loop_label "Could not resolve loop label"
#define err_msg_bad_self "Cannot use 'self' outside of a struct"
#define err_msg_bad_assign "Invalid assignment target"
#define err_msg_bad_static_fn "Expected function declaration after 'static'"
#define err_msg_bad_static_struct "Expected struct declaration after 'static'"
#define err_msg_bad_struct_member "Structs can only contain declarations"
#define err_msg_bad_struct_stat "Struct statement must be on line #1"
#define err_msg_bad_decl_scope(type) type " declaration must not be declared in a scope"
#define err_msg_bad_else_case "'else' case must be the last case in a switch"
#define err_msg_max_consts "Too many constants in one chunk"
#define err_msg_max_locals "Too many local variables in one chunk"
#define err_msg_max_upvals "Too many upvalues in one function"
#define err_msg_max_breaks "Too many break statements in one loop"
#define err_msg_max_cases "Too many cases in one switch"
#define err_msg_max_enum "Too many values in one enum"
#define err_msg_max_args "Too many arguments"
#define err_msg_max_param "Too many parameters"
#define err_msg_big_loop "Loop body too large"
#define err_msg_big_jmp "Jump body too large"
#define err_msg_expect_ident "Expected an identifier"
#define err_msg_expect_expr "Expected an expression"
#define err_msg_expect_eof "Expected EOF"
#define err_msg_expect_fn_body "Expected '{' or '=>'"
#define err_msg_expect(expected) "Expected '" expected "'"

#endif // __HBS_ERRMSG_H
