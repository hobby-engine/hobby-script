#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "errmsg.h"
#include "mem.h"
#include "chunk.h"
#include "lexer.h"
#include "obj.h"

#ifdef hby_print_bc
# include "debug.h"
#endif

typedef enum {
  Prec_none,
  Prec_assign,    // =
  Prec_or,        // ||
  Prec_and,       // &&
  Prec_eql,       // == !=
  Prec_is,        // is
  Prec_cmp,       // > < >= <=
  Prec_term,      // + - ..
  Prec_factor,    // * / %
  Prec_unary,     // ! -
  Prec_subscript, // []
  Prec_call,      // . () {}
  Prec_primary,
} Prec;

typedef void (*ParseFn)(Parser* p, bool can_assign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Prec prec;
} ParseRule;

typedef struct Loop {
  struct Loop* enclosing;

  bool is_for;

  int start; // The starting bytecode index of the loop
  int scope; // The scope depth that the loop lives in

  int breaks[uint8_count]; // Bytecode indices to break statements
  int breakc; // The amount of break statements

  bool named; // Is this loop named?
  Tok name; // What is the name?
} Loop;

// Represents a runtime value, at compile time
typedef struct {
  Tok name;
  int depth; // Scope depth
  bool captured; // Is this value captured by a closure?
  bool is_const; // Is this variable a constant?
} Local;

typedef struct {
  uint8_t index; // The index that the upvalue lives on the stack
  // Is this upvalue on a stack, or do we need to steal it from
  // an outer closure?
  bool is_local; 
  bool is_const; // Is this variable a constant?
} Upval;

typedef enum {
  FnType_fn,
  FnType_method,
  FnType_script,
} FnType;

typedef struct Compiler {
  struct Compiler* enclosing;
  GcFn* fn; // The function we're compiling to
  FnType type; // Type of function

  Local locals[uint8_count]; // Representation of the local scope's variables
  int localc; // Number of local variables

  Upval upvals[uint8_count]; // Captured upvalues

  int scope; // Current scope depth
  Loop* loop; // Innermost loop
} Compiler;

static Chunk* cur_chunk(Parser* p) {
  return &p->compiler->fn->chunk;
}

static int err_msg(char* buf, int len, Parser* p, Tok* t, const char* msg) {
  int total_len = snprintf(buf, len, "%s:%d", p->file_path, t->line);

  if (t->type == tok_eof) {
    total_len += snprintf(buf + total_len, len, " near end");
  } else if (t->type != tok_err) {
    total_len += snprintf(buf + total_len, len, " near '%.*s'", t->len, t->start);
  }

  total_len += snprintf(buf + total_len, len, ": %s", msg);
  return total_len;
}

static void err_at(Parser* p, Tok* t, const char* msg) {
  if (p->panic) {
    return;
  }
  p->panic = true;

  int len = err_msg(NULL, 0, p, t, msg);
  char* err_str = (char*)malloc(sizeof(char) * len + 1);
  err_msg(err_str, len, p, t, msg);

  hby_push_lstr(p->h, err_str, len);

  p->errc++;
}

static void err(Parser* p, const char* msg) {
  err_at(p, &p->prev, msg);
}

static void err_cur(Parser* p, const char* msg) {
  err_at(p, &p->cur, msg);
}

static void advance(Parser* p) {
  p->prev = p->cur;

  while (true) {
    p->cur = next_token(&p->lexer);
    if (p->cur.type != tok_err) {
      break;
    }

    err_cur(p, p->cur.start);
  }
}

static void expect(Parser* p, TokType type, const char* msg) {
  if (p->cur.type == type) {
    advance(p);
    return;
  }

  err_cur(p, msg);
}

static bool check(Parser* p, TokType type) {
  return p->cur.type == type;
}

static bool consume(Parser* p, TokType type) {
  if (!check(p, type)) {
    return false;
  }

  advance(p);
  return true;
}

static void write_bc(Parser* p, uint8_t byte) {
  write_chunk(p->h, cur_chunk(p), byte, p->prev.line);
}

static void write_2bc(Parser* p, uint8_t byte1, uint8_t byte2) {
  write_bc(p, byte1);
  write_bc(p, byte2);
}

static void write_loop(Parser* p, int loop_start) {
  write_bc(p, bc_loop);

  int jmp = cur_chunk(p)->len - loop_start + 2;
  if (jmp > UINT16_MAX) {
    err(p, err_msg_big_loop);
  }

  write_bc(p, (jmp >> 8) & 0xFF);
  write_bc(p, jmp & 0xFF);
}

static int write_jmp(Parser* p, uint8_t byte) {
  write_bc(p, byte);
  write_bc(p, 0xFF);
  write_bc(p, 0xFF);
  return cur_chunk(p)->len - 2;
}

static void patch_jmp(Parser* p, int idx) {
  int jmp = cur_chunk(p)->len - idx - 2;

  if (jmp > UINT16_MAX) {
    err(p, err_msg_big_jmp);
  }

  cur_chunk(p)->code[idx] = (jmp >> 8) & 0xFF;
  cur_chunk(p)->code[idx+1] = jmp & 0xFF;
}

static void write_ret(Parser* p) {
  write_bc(p, bc_null);
  write_bc(p, bc_ret);
}

static uint8_t create_const(Parser* p, Val val) {
  int c = add_const_chunk(p->h, cur_chunk(p), val);
  if (c > UINT8_MAX) {
    err(p, err_msg_max_consts);
    return 0;
  }
  return (uint8_t)c;
}

static void write_const(Parser* p, Val val) {
  write_2bc(p, bc_const, create_const(p, val));
}

static void init_compiler(Parser* p, Compiler* compiler, FnType type) {
  compiler->enclosing = p->compiler;

  compiler->fn = NULL;
  compiler->type = type;

  compiler->localc = 0;
  compiler->scope = 0;

  GcStr* path = copy_str(p->h, p->file_path, strlen(p->file_path));
  push(p->h, create_obj(path));
  compiler->fn = create_fn(p->h, path);
  pop(p->h);

  compiler->loop = NULL;

  p->compiler = compiler;

  if (type != FnType_script) {
    if (p->last_name_valid) {
      compiler->fn->name = copy_str(p->h, p->last_name.start, p->last_name.len);
    } else {
      compiler->fn->name = copy_str(p->h, "anonymous", 9);
    }
  }

  Local* local = &p->compiler->locals[p->compiler->localc++];
  local->depth = 0;
  local->captured = false;

  if (type != FnType_fn) {
    local->name.start = "self";
    local->name.len = 4;
  } else {
    local->name.start = "";
    local->name.len = 0;
  }
}

static GcFn* end_compiler(Parser* p) {
  write_ret(p);
  GcFn* fn = p->compiler->fn;

#ifdef hby_print_bc
  print_chunk(
    p->h, cur_chunk(p), fn->name != NULL ? fn->name->chars : "<script>");
#endif

  p->compiler = p->compiler->enclosing;
  return fn;
}

static int discard_locals(Parser* p, int scope) {
  int discarded = 0;

  while (p->compiler->localc - discarded > 0
      && p->compiler->locals[p->compiler->localc - discarded - 1].depth > scope) {
    if (p->compiler->locals[p->compiler->localc - discarded - 1].captured) {
      write_bc(p, bc_close_upval);
    } else {
      write_bc(p, bc_pop);
    }
    discarded++;
  }

  return discarded;
}

static void begin_loop(Parser* p, Loop* loop) {
  loop->start = cur_chunk(p)->len;
  loop->scope = p->compiler->scope;
  loop->enclosing = p->compiler->loop;
  loop->named = false;
  loop->breakc = 0;

  p->compiler->loop = loop;
}

static void end_loop(Parser* p) {
  Loop* loop = p->compiler->loop;

  for (int i = 0; i < loop->breakc; i++) {
    int break_idx = loop->breaks[i];
    cur_chunk(p)->code[break_idx - 1] = bc_jmp;
    patch_jmp(p, break_idx);
  }

  p->compiler->loop = loop->enclosing;
}

static bool idents_eql(Tok* a, Tok* b) {
  if (a->len != b->len) {
    return false;
  }

  return memcmp(a->start, b->start, a->len) == 0;
}

static Loop* resolve_loop(Parser* p, Tok name) {
  Loop* loop = p->compiler->loop;
  while (loop != NULL) {
    if (idents_eql(&name, &loop->name)) {
      return loop;
    }
    loop = loop->enclosing;
  }
  return NULL;
}

static void begin_scope(Parser* p) {
  p->compiler->scope++;
}

static void end_scope(Parser* p) {
  p->compiler->scope--;
  int discarded = discard_locals(p, p->compiler->scope);
  p->compiler->localc -= discarded;
}

static uint8_t ident_const(Parser* p, Tok* name) {
  return create_const(p, create_obj(copy_str(p->h, name->start, name->len)));
}

static int resolve_local(Parser* p, Compiler* compiler, Tok* name) {
  for (int i = compiler->localc - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (idents_eql(name, &local->name)) {
      if (local->depth == -1) {
        err(p, err_msg_self_read);
      }
      return i;
    }
  }
  return -1;
}

static int add_upvalue(Parser* p, Compiler* compiler, uint8_t index, bool is_local) {
  int upvalc = compiler->fn->upvalc;

  for (int i = 0; i < upvalc; i++) {
    Upval* upval = &compiler->upvals[i];
    if (upval->index == index && upval->is_local == is_local) {
      return i;
    }
  }

  if (upvalc == uint8_count) {
    err(p, err_msg_max_upvals);
    return 0;
  }

  compiler->upvals[upvalc].is_local = is_local;
  compiler->upvals[upvalc].is_const = compiler->locals[index].is_const;
  compiler->upvals[upvalc].index = index;
  return compiler->fn->upvalc++;
}

static int resolve_upval(Parser* p, Compiler* compiler, Tok* name) {
  // Topmost code; no upvalues to resolve.
  if (compiler->enclosing == NULL) {
    return -1;
  }

  // Find locals in the outer scope.
  int local = resolve_local(p, compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].captured = true;
    return add_upvalue(p, compiler, (uint8_t)local, true);
  }

  // Steal upvalues from the outer scope.
  int upval = resolve_upval(p, compiler->enclosing, name);
  if (upval != -1) {
    return add_upvalue(p, compiler, (uint8_t)upval, false);
  }

  // Could not find variable to capture
  return -1;
}

static void add_local(Parser* p, Tok name, bool is_const) {
  if (p->compiler->localc == uint8_count) {
    err(p, err_msg_max_locals);
    return;
  }

  Local* local = &p->compiler->locals[p->compiler->localc++];
  local->name = name;
  local->depth = -1;
  local->captured = false;
  local->is_const = is_const;
}

static void decl_var(Parser* p, bool is_const) {
  Tok* name = &p->prev;

  for (int i = p->compiler->localc - 1; i >= 0; i--) {
    Local* local = &p->compiler->locals[i];
    if (local->depth != -1 && local->depth < p->compiler->scope) {
      break;
    }

    if (idents_eql(name, &local->name)) {
      err(p, err_msg_shadow);
    }
  }

  add_local(p, *name, is_const);
}

static void parse_var(Parser* p, const char* msg, bool is_const) {
  expect(p, tok_ident, msg);

  p->last_name_valid = true;
  p->last_name = p->prev;

  decl_var(p, is_const);
}

static void mark_init(Parser* p) {
  p->compiler->locals[p->compiler->localc - 1].depth =
    p->compiler->scope;
}

//
// EXPRESSIONS
//

static void expr(Parser* p);
static void decl(Parser* p);
static void stat(Parser* p);
static void block(Parser* p);
static void parse_prec(Parser* p, Prec prec);
static ParseRule* get_rule(TokType type);

static void and_expr(Parser* p, bool can_assign) {
  int end_jmp = write_jmp(p, bc_false_jmp);
  write_bc(p, bc_pop);
  parse_prec(p, Prec_and);
  patch_jmp(p, end_jmp);
}

static void or_expr(Parser* p, bool can_assign) {
  int else_jmp = write_jmp(p, bc_false_jmp);
  int end_jmp = write_jmp(p, bc_jmp);
  patch_jmp(p, else_jmp);

  write_bc(p, bc_pop);
  parse_prec(p, Prec_or);
  patch_jmp(p, end_jmp);
}

static void binary_expr(Parser* p, bool can_assign) {
  TokType op = p->prev.type;
  ParseRule* rule = get_rule(op);
  parse_prec(p, (Prec)(rule->prec + 1));

  switch (op) {
    case tok_plus: write_bc(p, bc_add); break;
    case tok_minus: write_bc(p, bc_sub); break;
    case tok_star: write_bc(p, bc_mul); break;
    case tok_slash: write_bc(p, bc_div); break;
    case tok_percent: write_bc(p, bc_mod); break;
    case tok_gt: write_bc(p, bc_gt); break;
    case tok_gte: write_bc(p, bc_gte); break;
    case tok_lt: write_bc(p, bc_lt); break;
    case tok_lte: write_bc(p, bc_lte); break;
    case tok_eql_eql: write_bc(p, bc_eql); break;
    case tok_bang_eql: write_bc(p, bc_neql); break;
    case tok_dot_dot: write_bc(p, bc_cat); break;
    case tok_is: write_bc(p, bc_is); break;
    default: return;
  }
}

static void literal_expr(Parser* p, bool can_assign) {
  switch (p->prev.type) {
    case tok_false: write_bc(p, bc_false); break;
    case tok_true: write_bc(p, bc_true); break;
    case tok_null: write_bc(p, bc_null); break;
    default: return;
  }
}

static void grouping_expr(Parser* p, bool can_assign) {
  expr(p);
  expect(p, tok_rparen, err_msg_expect(")"));
}

static void num_expr(Parser* p, bool can_assign) {
  double val = strtod(p->prev.start, NULL);
  write_const(p, create_num(val));
}

static void str_expr(Parser* p, bool can_assign) {
  write_const(p, create_obj(as_obj(p->prev.val)));
}

static void strfmt_expr(Parser* p, bool can_assign) {
  int count = 0;

  do {
    write_const(p, create_obj(as_obj(p->prev.val)));
    expr(p);
    write_bc(p, bc_cat);
    count++;
  } while (consume(p, tok_strfmt));

  expect(p, tok_strfmt_end, err_msg_eof_str);
  write_const(p, create_obj(as_obj(p->prev.val)));

  for (int i = 0; i < count; i++) {
    write_bc(p, bc_cat);
  }
}

static void map_expr(Parser* p, bool can_assign) {
  write_bc(p, bc_map);

  while (!check(p, tok_rbrace) && !check(p, tok_eof)) {
    expr(p);
    expect(p, tok_rarrow, err_msg_expect("->"));
    expr(p);

    write_bc(p, bc_map_item);
    if (!consume(p, tok_comma) && !check(p, tok_rbrace)) {
      err(p, err_msg_expect(","));
    }
  }

  expect(p, tok_rbrace, err_msg_expect("}"));
}

static void array_expr(Parser* p, bool can_assign) {
  write_bc(p, bc_array);

  while (!check(p, tok_rbracket) && !check(p, tok_eof)) {
    expr(p);

    write_bc(p, bc_array_item);
    if (!consume(p, tok_comma) && !check(p, tok_rbracket)) {
      err(p, err_msg_expect(","));
    }
  }

  expect(p, tok_rbracket, err_msg_expect("]"));
}

static uint8_t arg_list(Parser* p) {
  bool in_expr_stat = p->in_expr_stat;
  p->in_expr_stat = false;
  uint8_t argc = 0;
  if (!check(p, tok_rparen)) {
    do {
      expr(p);
      if (argc == 255) {
        err(p, err_msg_max_args);
      }

      argc++;
    } while (consume(p, tok_comma));
  }
  p->in_expr_stat = in_expr_stat;
  expect(p, tok_rparen, err_msg_expect(")"));
  return argc;
}

static void call_expr(Parser* p, bool can_assign) {
  uint8_t argc = arg_list(p);
  write_2bc(p, bc_call, argc);
}

static void instance_expr(Parser* p, bool can_assign) {
  write_bc(p, bc_inst);

  while (!check(p, tok_rbrace) && !check(p, tok_eof)) {
    expect(p, tok_ident, err_msg_expect_ident);
    Tok name = p->prev;

    expect(p, tok_eql, err_msg_expect("="));

    expr(p);
    write_2bc(p, bc_init_prop, ident_const(p, &name));

    if (!consume(p, tok_comma) && !check(p, tok_rbrace)) {
      err(p, err_msg_expect(","));
    }
  } 

  expect(p, tok_rbrace, err_msg_expect("}"));
}

static void dot_expr(Parser* p, bool can_assign) {
  expect(p, tok_ident, err_msg_expect("."));
  uint8_t name = ident_const(p, &p->prev);

#define shorthand_op(op) \
  do { \
    write_2bc(p, bc_push_prop, name); \
    expr(p); \
    write_bc(p, op); \
    write_2bc(p, bc_set_prop, name); \
  } while (false)
#define compound_op(op) \
  do { \
    write_2bc(p, bc_push_prop, name); \
    write_2bc(p, bc_const, create_const(p, create_num(1))); \
    write_bc(p, op); \
    write_2bc(p, bc_set_prop, name); \
  } while (false)

  if (can_assign && consume(p, tok_eql)) {
    expr(p);
    write_2bc(p, bc_set_prop, name);
  } else if (consume(p, tok_lparen)) {
    uint8_t argc = arg_list(p);
    write_2bc(p, bc_invoke, name);
    write_bc(p, argc);
  } else if (can_assign && consume(p, tok_plus_eql)) {
    shorthand_op(bc_add);
  } else if (can_assign && consume(p, tok_minus_eql)) {
    shorthand_op(bc_sub);
  } else if (can_assign && consume(p, tok_star_eql)) {
    shorthand_op(bc_mul);
  } else if (can_assign && consume(p, tok_slash_eql)) {
    shorthand_op(bc_div);
  } else if (can_assign && consume(p, tok_percent_eql)) {
    shorthand_op(bc_mod);
  } else if (can_assign && consume(p, tok_plus_plus)) {
    compound_op(bc_add);
  } else if (can_assign && consume(p, tok_minus_minus)) {
    compound_op(bc_sub);
  } else {
    write_2bc(p, bc_get_prop, name);
  }

#undef shorthand_op
#undef compound_op
}

static void subscript_expr(Parser* p, bool can_assign) {
  expr(p);
  expect(p, tok_rbracket, err_msg_expect("]"));

#define shorthand_op(op) \
  do { \
    write_bc(p, bc_push_subscript); \
    expr(p); \
    write_bc(p, op); \
    write_bc(p, bc_set_subscript); \
  } while (false)
#define compound_op(op) \
  do { \
    write_bc(p, bc_push_subscript); \
    write_2bc(p, bc_const, create_const(p, create_num(1))); \
    write_bc(p, op); \
    write_bc(p, bc_set_subscript); \
  } while (false)

  if (can_assign && consume(p, tok_eql)) {
    expr(p);
    write_bc(p, bc_set_subscript);
  } else if (can_assign && consume(p, tok_plus_eql)) {
    shorthand_op(bc_add);
  } else if (can_assign && consume(p, tok_minus_eql)) {
    shorthand_op(bc_sub);
  } else if (can_assign && consume(p, tok_star_eql)) {
    shorthand_op(bc_mul);
  } else if (can_assign && consume(p, tok_slash_eql)) {
    shorthand_op(bc_div);
  } else if (can_assign && consume(p, tok_percent_eql)) {
    shorthand_op(bc_mod);
  } else if (can_assign && consume(p, tok_plus_plus)) {
    compound_op(bc_add);
  } else if (can_assign && consume(p, tok_minus_minus)) {
    compound_op(bc_sub);
  } else {
    write_bc(p, bc_get_subscript);
  }
#undef shorthand_op
#undef compound_op
}

static void static_dot_expr(Parser* p, bool can_assign) {
  expect(p, tok_ident, err_msg_expect_ident);
  uint8_t name = ident_const(p, &p->prev);
  write_2bc(p, bc_get_static, name);
}

static void function(Parser* p, FnType type, bool is_lambda);
static void function_expr(Parser* p, bool can_assign) {
  function(p, FnType_fn, true);
}

static void check_const(Parser* p, uint8_t setter, int arg) {
  switch (setter) {
    case bc_set_local:
      if (p->compiler->locals[arg].is_const) {
        err(p, err_msg_assign_const);
      }
      break;
    case bc_set_upval:
      if (p->compiler->upvals[arg].is_const) {
        err(p, err_msg_assign_const);
      }
      break;
    case bc_get_global:
      err(p, err_msg_assign_const);
    default:
      break;
  }
}

static void named_var(Parser* p, Tok name, bool can_assign) {
  if (p->in_expr_stat && consume(p, tok_comma)) { // Multiple assignment
    Tok names[UINT8_MAX];
    names[0] = name;
    int count = 1;

    do {
      expect(p, tok_ident, err_msg_expect_ident);
      names[count] = p->prev;
      count++;
    } while (consume(p, tok_comma));

    if (!can_assign) {
      err(p, err_msg_bad_assign);
    }
    expect(p, tok_eql, err_msg_expect("="));

    expr(p);

    for (int i = 0; i < count; i++) {
      uint8_t setter;
      int arg = resolve_local(p, p->compiler, &names[i]);
      if (arg != -1) {
        setter = bc_set_local;
      } else if ((arg = resolve_upval(p, p->compiler, &names[i])) != -1) {
        setter = bc_set_upval;
      } else {
        arg = ident_const(p, &names[i]);
        GcStr* name = as_str(p->compiler->fn->chunk.consts.items[arg]);
        Val v;
        if (get_table(&p->h->global_consts, name, &v)) {
          err(p, err_msg_assign_const);
        }
        setter = bc_get_global;
      }

      write_2bc(p, bc_destruct_array, i);
      write_2bc(p, setter, (uint8_t)arg);
      write_bc(p, bc_pop);
    }

    return;
  }

  uint8_t getter, setter;
  int arg = resolve_local(p, p->compiler, &name);
  if (arg != -1) {
    getter = bc_get_local;
    setter = bc_set_local;
  } else if ((arg = resolve_upval(p, p->compiler, &name)) != -1) {
    getter = bc_get_upval;
    setter = bc_set_upval;
  } else {
    arg = ident_const(p, &name);
    getter = bc_get_global;
    setter = bc_get_global;
  }

#define shorthand_op(op) \
  do { \
    check_const(p, setter, arg); \
    write_2bc(p, getter, (uint8_t)arg); \
    expr(p); \
    write_bc(p, op); \
    write_2bc(p, setter, (uint8_t)arg); \
  } while (false)
#define compound_op(op) \
  do { \
    check_const(p, setter, arg); \
    write_2bc(p, getter, (uint8_t)arg); \
    write_2bc(p, bc_const, create_const(p, create_num(1))); \
    write_bc(p, op); \
    write_2bc(p, setter, (uint8_t)arg); \
  } while (false)

  if (can_assign && consume(p, tok_eql)) {
    check_const(p, setter, arg);
    expr(p);
    write_2bc(p, setter, (uint8_t)arg);
  } else if (can_assign && consume(p, tok_plus_eql)) {
    shorthand_op(bc_add);
  } else if (can_assign && consume(p, tok_minus_eql)) {
    shorthand_op(bc_sub);
  } else if (can_assign && consume(p, tok_star_eql)) {
    shorthand_op(bc_mul);
  } else if (can_assign && consume(p, tok_slash_eql)) {
    shorthand_op(bc_div);
  } else if (can_assign && consume(p, tok_percent_eql)) {
    shorthand_op(bc_mod);
  } else if (can_assign && consume(p, tok_plus_plus)) {
    compound_op(bc_add);
  } else if (can_assign && consume(p, tok_minus_minus)) {
    compound_op(bc_sub);
  } else {
    write_2bc(p, getter, (uint8_t)arg);
  }

#undef shorthand_op
#undef compound_op
}

static void var_expr(Parser* p, bool can_assign) {
  named_var(p, p->prev, can_assign);
}

static void self_expr(Parser* p, bool can_assign) {
  if (!p->within_struct) {
    err(p, err_msg_bad_self);
    return;
  }

  var_expr(p, false);
}

static void ternary_expr(Parser* p, bool can_assign) {
  expect(p, tok_lparen, err_msg_expect("("));
  expr(p);
  expect(p, tok_rparen, err_msg_expect(")"));

  int then_jmp = write_jmp(p, bc_false_jmp);
  write_bc(p, bc_pop);

  expr(p);

  int else_jmp = write_jmp(p, bc_jmp);

  expect(p, tok_else, err_msg_expect("else"));
  patch_jmp(p, then_jmp);
  write_bc(p, bc_pop);

  expr(p);

  patch_jmp(p, else_jmp);
}

static void unary_expr(Parser* p, bool can_assign) {
  TokType op = p->prev.type;

  parse_prec(p, Prec_unary);

  switch (op) {
    case tok_minus: write_bc(p, bc_neg); break;
    case tok_bang: write_bc(p, bc_not); break;
    default: return;
  }
}

ParseRule rules[] = {
  [tok_lparen]      = {grouping_expr, call_expr, Prec_call},
  [tok_rparen]      = {NULL, NULL, Prec_none},
  [tok_lbrace]      = {map_expr, instance_expr, Prec_call},
  [tok_rbrace]      = {NULL, NULL, Prec_none},
  [tok_lbracket]    = {array_expr, subscript_expr, Prec_subscript},
  [tok_rbracket]    = {NULL, NULL, Prec_none},
  [tok_comma]       = {NULL, NULL, Prec_none},
  [tok_dot]         = {NULL, dot_expr, Prec_call},
  [tok_minus]       = {unary_expr, binary_expr, Prec_term},
  [tok_plus]        = {NULL, binary_expr, Prec_term},
  [tok_star]        = {NULL, binary_expr, Prec_factor},
  [tok_slash]       = {NULL, binary_expr, Prec_factor},
  [tok_semicolon]   = {NULL, NULL, Prec_none},
  [tok_bang]        = {unary_expr, NULL, Prec_none},
  [tok_lt]          = {NULL, binary_expr, Prec_cmp},
  [tok_gt]          = {NULL, binary_expr, Prec_cmp},
  [tok_eql]         = {NULL, NULL, Prec_none},
  [tok_colon]       = {NULL, static_dot_expr, Prec_call},
  [tok_eql_eql]     = {NULL, binary_expr, Prec_eql},
  [tok_bang_eql]    = {NULL, binary_expr, Prec_eql},
  [tok_gte]         = {NULL, binary_expr, Prec_cmp},
  [tok_lte]         = {NULL, binary_expr, Prec_cmp},
  [tok_and]         = {NULL, and_expr, Prec_and},
  [tok_or]          = {NULL, or_expr, Prec_or},
  [tok_rarrow]      = {NULL, NULL, Prec_none},
  [tok_dot_dot]     = {NULL, binary_expr, Prec_term},
  [tok_plus_plus]   = {NULL, NULL, Prec_none},
  [tok_minus_minus] = {NULL, NULL, Prec_none},
  [tok_ident]       = {var_expr, NULL, Prec_none},
  [tok_str]         = {str_expr, NULL, Prec_none},
  [tok_strfmt]      = {strfmt_expr, NULL, Prec_none},
  [tok_num]         = {num_expr, NULL, Prec_none},
  [tok_true]        = {literal_expr, NULL, Prec_none},
  [tok_false]       = {literal_expr, NULL, Prec_none},
  [tok_null]        = {literal_expr, NULL, Prec_none},
  [tok_struct]      = {NULL, NULL, Prec_none},
  [tok_if]          = {ternary_expr, NULL, Prec_none},
  [tok_is]          = {NULL, binary_expr, Prec_is},
  [tok_else]        = {NULL, NULL, Prec_none},
  [tok_while]       = {NULL, NULL, Prec_none},
  [tok_for]         = {NULL, NULL, Prec_none},
  [tok_loop]        = {NULL, NULL, Prec_none},
  [tok_static]      = {NULL, NULL, Prec_none},
  [tok_fn]          = {function_expr, NULL, Prec_none},
  [tok_enum]        = {NULL, NULL, Prec_none},
  [tok_self]        = {self_expr, NULL, Prec_none},
  [tok_return]      = {NULL, NULL, Prec_none},
  [tok_var]         = {NULL, NULL, Prec_none},
  [tok_const]       = {NULL, NULL, Prec_none},
  [tok_switch]      = {NULL, NULL, Prec_none},
  [tok_case]        = {NULL, NULL, Prec_none},
  [tok_break]       = {NULL, NULL, Prec_none},
  [tok_continue]    = {NULL, NULL, Prec_none},
  [tok_plus_eql]    = {NULL, NULL, Prec_none},
  [tok_minus_eql]   = {NULL, NULL, Prec_none},
  [tok_star_eql]    = {NULL, NULL, Prec_none},
  [tok_slash_eql]   = {NULL, NULL, Prec_none},
  [tok_percent_eql] = {NULL, NULL, Prec_none},
  [tok_percent]     = {NULL, binary_expr, Prec_factor},
  [tok_err]         = {NULL, NULL, Prec_none},
  [tok_eof]         = {NULL, NULL, Prec_none},
};

static void parse_prec(Parser* p, Prec prec) {
  advance(p);

  ParseFn prefix = get_rule(p->prev.type)->prefix;
  if (prefix == NULL) {
    err(p, err_msg_expect_expr);
    return;
  }

  bool can_assign = prec <= Prec_assign;
  prefix(p, can_assign);

  while (prec <= get_rule(p->cur.type)->prec) {
    advance(p);
    ParseFn infix = get_rule(p->prev.type)->infix;
    infix(p, can_assign);
  }

  if (can_assign && consume(p, tok_eql)) {
    err(p, err_msg_bad_assign);
  }
}

static ParseRule* get_rule(TokType type) {
  return &rules[type];
}

static void expr(Parser* p) {
  parse_prec(p, Prec_assign);
}

static void block(Parser* p) {
  while (!check(p, tok_rbrace) && !check(p, tok_eof)) {
    decl(p);
  }

  expect(p, tok_rbrace, err_msg_expect("}"));
}

//
// DECLARATIONS
//

static void var_decl(Parser* p, bool is_const) {
  Tok names[UINT8_MAX];
  int count = 0;

  do {
    if (count == UINT8_MAX) {
      err(p, err_msg_max_destruct);
      return;
    }

    Tok name = p->cur;
    parse_var(p, err_msg_expect_ident, is_const);
    names[count] = name;
    count++;

    if (check(p, tok_comma) || count > 1) {
      mark_init(p);
      write_bc(p, bc_null); // Reserve this slot
    }
  } while (consume(p, tok_comma));

  if (consume(p, tok_eql)) {
    expr(p);
  } else {
    write_bc(p, bc_null);
  }

  if (count > 1) { // Multiple assignment
    for (int i = 0; i < count; i++) {
      write_2bc(p, bc_destruct_array, i);
      mark_init(p);

      uint8_t local = resolve_local(p, p->compiler, &names[i]);
      write_2bc(p, bc_set_local, local);
      write_bc(p, bc_pop);
    }
    write_bc(p, bc_pop); // The expression result
  } else { // Single assignment

    mark_init(p);
  }

  expect(p, tok_semicolon, err_msg_expect(";"));
}

static void function(Parser* p, FnType type, bool is_lambda) {
  mark_init(p);

  Compiler compiler;
  init_compiler(p, &compiler, type);
  // begin_scope(p);

  expect(p, tok_lparen, err_msg_expect("("));
  if (!check(p, tok_rparen)) {
    do {
      if (p->compiler->fn->arity == UINT8_MAX) {
        err_cur(p, err_msg_max_param);
      }
      p->compiler->fn->arity++;
      parse_var(p, err_msg_expect_ident, false);
      mark_init(p);
    } while (consume(p, tok_comma));
  }
  expect(p, tok_rparen, err_msg_expect(")"));

  if (consume(p, tok_rarrow)) {
    expr(p);
    write_bc(p, bc_ret);

    if (!is_lambda) {
      // When a function is not a lambda, it doesn't check for a semicolon.
      expect(p, tok_semicolon, err_msg_expect(";"));
    }
  } else if (consume(p, tok_lbrace)) {
    block(p);
  } else {
    err(p, err_msg_expect_fn_body);
  }

  GcFn* fn = end_compiler(p);
  write_2bc(p, bc_closure, create_const(p, create_obj(fn)));

  for (int i = 0; i < fn->upvalc; i++) {
    write_bc(p, compiler.upvals[i].is_local ? 1 : 0);
    write_bc(p, compiler.upvals[i].index);
  }
}

static void function_decl(Parser* p) {
  parse_var(p, err_msg_expect_ident, true);
  mark_init(p);
  function(p, FnType_fn, false);
}

static uint8_t method(Parser* p, bool is_static) {
  expect(p, tok_ident, err_msg_expect_ident);

  Tok name = p->prev;
  p->last_name_valid = true;
  p->last_name = name;

  uint8_t name_const = ident_const(p, &name);

  FnType type = is_static ? FnType_fn : FnType_method;
  function(p, type, false);
  if (!is_static) {
    write_2bc(p, bc_method, name_const);
  }
  return name_const;
}

static void sub_struct_decl(Parser* p, bool all_static);
static void enum_body(Parser* p, uint8_t name_const);

static void struct_body(Parser* p, bool all_static) {
  bool is_static = all_static;
  uint8_t static_name = 0;

  if (consume(p, tok_fn)) { // Member functions
    static_name = method(p, all_static);
  } else if (consume(p, tok_var)) { // Member variables
    // TODO: Error here on `all_static`
    expect(p, tok_ident, err_msg_expect_ident);
    Tok name = p->prev;

    if (consume(p, tok_eql)) {
      expr(p);
    } else {
      write_bc(p, bc_null);
    }

    write_2bc(p, bc_member, ident_const(p, &name));
    expect(p, tok_semicolon, err_msg_expect(";"));
  } else if (consume(p, tok_const)) { // Static consts
    expect(p, tok_ident, err_msg_expect_ident);
    Tok name = p->prev;

    if (consume(p, tok_eql)) {
      expr(p);
    } else {
      write_bc(p, bc_null);
    }

    static_name = ident_const(p, &name);
    expect(p, tok_semicolon, err_msg_expect(";"));
    is_static = true;
  } else if (consume(p, tok_static)) { // Static functions or sub-structs
    if (consume(p, tok_fn)) { // Static functions
      static_name = method(p, true);
    } else if (consume(p, tok_struct)) { // Static structs
      expect(p, tok_ident, err_msg_expect_ident);
      static_name = ident_const(p, &p->prev);
      write_2bc(p, bc_struct, static_name);
      sub_struct_decl(p, true);
    } else {
      err(p, err_msg_bad_static_member);
    }
    is_static = true;
  } else if (consume(p, tok_enum)) { // Enums
    expect(p, tok_ident, err_msg_expect_ident);
    static_name = ident_const(p, &p->prev);
    enum_body(p, static_name);
    is_static = true;
  } else if (consume(p, tok_struct)) { // Sub-structs
    expect(p, tok_ident, err_msg_expect_ident);
    static_name = ident_const(p, &p->prev);
    write_2bc(p, bc_struct, static_name);
    sub_struct_decl(p, false);
    is_static = true;
  } else {
    err_cur(p, err_msg_bad_struct_member);
    advance(p);
  }

  if (is_static) {
    write_2bc(p, bc_def_static, static_name);
  }
}

static void sub_struct_decl(Parser* p, bool all_static) {
  expect(p, tok_lbrace, err_msg_expect("{"));

  while (!check(p, tok_rbrace) && !check(p, tok_eof)) {
    struct_body(p, all_static);
  }
  expect(p, tok_rbrace, err_msg_expect("}"));
}

static void struct_decl(Parser* p, bool all_static) {
  if (p->compiler->type != FnType_script || p->compiler->scope != 0) {
    err(p, err_msg_bad_decl_scope("struct"));
  }

  p->within_struct = true;

  expect(p, tok_ident, err_msg_expect_ident);
  Tok name = p->prev;
  uint8_t name_const = ident_const(p, &name);
  decl_var(p, true);

  write_2bc(p, bc_struct, name_const);
  mark_init(p);
  
  named_var(p, name, false);

  // If we see a semicolon, then that means this whole file is a struct
  // declaration
  if (consume(p, tok_semicolon)) {
    if (p->prev.line != 1) {
      err(p, err_msg_bad_struct_stat);
    }

    while (!check(p, tok_eof)) {
      struct_body(p, all_static);
    }

    // Return the struct from the file
    write_bc(p, bc_ret);
  } else {
    expect(p, tok_lbrace, err_msg_expect("{"));
    while (!check(p, tok_rbrace) && !check(p, tok_eof)) {
      struct_body(p, all_static);
    }
    expect(p, tok_rbrace, err_msg_expect("}"));
    write_bc(p, bc_pop);
  }

  p->within_struct = false;
}

static void enum_body(Parser* p, uint8_t name_const) {
  write_2bc(p, bc_enum, name_const);

  uint8_t enum_names[UINT8_MAX];
  int enumc = 0;

  expect(p, tok_lbrace, err_msg_expect("{"));
  while (!check(p, tok_rbrace) && !check(p, tok_eof)) {
    expect(p, tok_ident, err_msg_expect_ident);
    if (enumc == UINT8_MAX) {
      err(p, err_msg_max_enum);
      return;
    }
    uint8_t enum_name = ident_const(p, &p->prev);
    enum_names[enumc++] = enum_name;
    expect(p, tok_comma, err_msg_expect(","));
  }
  expect(p, tok_rbrace, err_msg_expect("}"));

  write_bc(p, enumc);
  for (int i = 0; i < enumc; i++) {
    write_bc(p, enum_names[i]);
  }
}

static void enum_decl(Parser* p) {
  if (p->compiler->type != FnType_script || p->compiler->scope != 0) {
    err(p, err_msg_bad_decl_scope("enum"));
  }

  expect(p, tok_ident, err_msg_expect_ident);
  Tok name = p->prev;
  uint8_t name_const = ident_const(p, &name);
  decl_var(p, true);
  mark_init(p);

  enum_body(p, name_const);
}

//
// STATEMENTS
//

static void expr_stat(Parser* p) {
  p->in_expr_stat = true;
  expr(p);
  p->in_expr_stat = false;
  expect(p, tok_semicolon, err_msg_expect(";"));
  write_bc(p, bc_pop);
}

static void if_stat(Parser* p) {
  expect(p, tok_lparen, err_msg_expect("("));
  expr(p);
  expect(p, tok_rparen, err_msg_expect(")"));

  int then_jmp = write_jmp(p, bc_false_jmp);
  write_bc(p, bc_pop);
  stat(p);

  int else_jmp = write_jmp(p, bc_jmp);

  patch_jmp(p, then_jmp);
  write_bc(p, bc_pop);

  if (consume(p, tok_else)) {
    stat(p);
  }

  patch_jmp(p, else_jmp);

}

static void continue_stat(Parser* p) {
  if (p->compiler->loop == NULL) {
    err(p, err_msg_use_continue);
    return;
  }

  Loop* loop = p->compiler->loop;
  if (consume(p, tok_ident)) {
    loop = resolve_loop(p, p->prev);
    if (loop == NULL) {
      err(p, err_msg_bad_loop_label);
      return;
    }
  }

  discard_locals(p, loop->scope);
  write_loop(p, loop->start);

  expect(p, tok_semicolon, err_msg_expect(";"));
}

static void break_stat(Parser* p) {
  if (p->compiler->loop == NULL) {
    err(p, err_msg_use_break);
    return;
  }

  Loop* loop = p->compiler->loop;

  if (consume(p, tok_ident)) {
    loop = resolve_loop(p, p->prev);
    if (loop == NULL) {
      err(p, err_msg_bad_loop_label);
      return;
    }
  }

  if (loop->is_for) {
    discard_locals(p, loop->scope - 1);
  } else {
    discard_locals(p, loop->scope);
  }

  int index = write_jmp(p, bc_break);

  if (loop->breakc == uint8_count) {
    err(p, err_msg_max_breaks);
    return;
  }

  loop->breaks[loop->breakc++] = index;

  expect(p, tok_semicolon, err_msg_expect(";"));
}

static void loop_stat(Parser* p) {
  Loop loop;
  begin_loop(p, &loop);

  if (consume(p, tok_colon)) {
    expect(p, tok_ident, err_msg_expect_ident);
    loop.name = p->prev;
  }

  stat(p);

  write_loop(p, loop.start);

  end_loop(p);
}

static void while_stat(Parser* p) {
  Loop loop;
  begin_loop(p, &loop);

  expect(p, tok_lparen, err_msg_expect("("));
  expr(p);
  expect(p, tok_rparen, err_msg_expect(")"));

  if (consume(p, tok_colon)) {
    expect(p, tok_ident, err_msg_expect_ident);
    loop.name = p->prev;
  }

  int exit_jmp = write_jmp(p, bc_false_jmp);
  write_bc(p, bc_pop);

  stat(p);

  write_loop(p, loop.start);

  patch_jmp(p, exit_jmp);
  write_bc(p, bc_pop);

  end_loop(p);
}

static void for_stat(Parser* p) {
  begin_scope(p);

  // Loop variable
  expect(p, tok_lparen, err_msg_expect("("));
  if (!consume(p, tok_semicolon)) {
    var_decl(p, false);
  }

  Loop loop;
  begin_loop(p, &loop);
  loop.is_for = true;

  // Loop condition
  int exit_jmp = -1;
  if (!consume(p, tok_semicolon)) {
    expr(p);
    expect(p, tok_semicolon, err_msg_expect(";"));

    exit_jmp = write_jmp(p, bc_false_jmp);
    write_bc(p, bc_pop);
  }

  // Increment
  if (!consume(p, tok_rparen)) {
    int body_jmp = write_jmp(p, bc_jmp);
    int inc_start = cur_chunk(p)->len;

    expr(p);
    write_bc(p, bc_pop);
    expect(p, tok_rparen, err_msg_expect(")"));

    write_loop(p, loop.start);
    loop.start = inc_start;
    patch_jmp(p, body_jmp);
  }

  if (consume(p, tok_colon)) {
    expect(p, tok_ident, err_msg_expect_ident);
    loop.name = p->prev;
  }

  stat(p);
  write_loop(p, loop.start);

  if (exit_jmp != -1) {
    patch_jmp(p, exit_jmp);
    write_bc(p, bc_pop);
  }

  end_scope(p);
  end_loop(p);
}

static void return_stat(Parser* p) {
  if (consume(p, tok_semicolon)) {
    write_ret(p);
  } else {
    expr(p);
    expect(p, tok_semicolon, err_msg_expect(";"));
    write_bc(p, bc_ret);
  }
}

static void switch_stat(Parser* p) {
  expect(p, tok_lparen, err_msg_expect("("));
  expr(p);
  expect(p, tok_rparen, err_msg_expect(")"));

  int cases[UINT8_MAX];
  int casec = 0;

  expect(p, tok_lbrace, err_msg_expect("{"));

  if (consume(p, tok_case)) {
    do {
      expr(p); // The value to switch on
      int ineq_jmp = write_jmp(p, bc_ineq_jmp);

      write_bc(p, bc_pop); // switch expression

      expect(p, tok_rarrow, err_msg_expect("->"));
      stat(p);

      if (casec == UINT8_MAX) {
        err(p, err_msg_max_cases);
      }

      cases[casec++] = write_jmp(p, bc_jmp);
      patch_jmp(p, ineq_jmp);
    } while (consume(p, tok_case));
  }

  if (consume(p, tok_else)) {
    expect(p, tok_rarrow, err_msg_expect("->"));
    write_bc(p, bc_pop); // switch expression
    stat(p);
  }

  if (consume(p, tok_case)) {
    err(p, err_msg_bad_else_case);
    while (!consume(p, tok_rbrace)) advance(p);
  }

  for (int i = 0; i < casec; i++) {
    patch_jmp(p, cases[i]);
  }

  expect(p, tok_rbrace, err_msg_expect("}"));
}

static void sync(Parser* p) {
  p->panic = false;

  while (p->cur.type != tok_eof) {
    if (p->prev.type == tok_semicolon) {
      return;
    }

    switch (p->cur.type) {
      case tok_struct:
      case tok_enum:
      case tok_fn:
      case tok_var:
      case tok_const:
      case tok_static:
      case tok_if:
      case tok_else:
      case tok_for:
      case tok_while:
      case tok_loop:
      case tok_return:
      case tok_continue:
      case tok_break:
      case tok_switch:
      case tok_case:
      case tok_rbrace:
        return;
      default:
        break;
    }

    advance(p);
  }
}

static void decl(Parser* p) {
  if (consume(p, tok_static)) {
    expect(p, tok_struct, err_msg_bad_static_struct);
    struct_decl(p, true);
  } else if (consume(p, tok_var)) {
    var_decl(p, false);
  } else if (consume(p, tok_const)) {
    var_decl(p, true);
  } else if (consume(p, tok_struct)) {
    struct_decl(p, false);
  } else if (consume(p, tok_enum)) {
    enum_decl(p);
  } else if (consume(p, tok_fn)) {
    function_decl(p);
  } else {
    stat(p);
  }

  if (p->panic) {
    sync(p);
  }
}

static void stat(Parser* p) {
  p->last_name_valid = false;

  if (consume(p, tok_break)) {
    break_stat(p);
  } else if (consume(p, tok_continue)) {
    continue_stat(p);
  } else if (consume(p, tok_if)) {
    if_stat(p);
  } else if (consume(p, tok_loop)) {
    loop_stat(p);
  } else if (consume(p, tok_for)) {
    for_stat(p);
  } else if (consume(p, tok_while)) {
    while_stat(p);
  } else if (consume(p, tok_switch)) {
    switch_stat(p);
  } else if (consume(p, tok_return)) {
    return_stat(p);
  } else if (consume(p, tok_lbrace)) {
    begin_scope(p);
    block(p);
    end_scope(p);
  } else {
    expr_stat(p);
  }
}

GcFn* compile_hby(hby_State* h, const char* path, const char* src) {
  Parser* p = h->parser;
  p->h = h;
  p->file_path = path;
  p->compiler = NULL;

  init_lexer(h, &p->lexer, src);
  
  Compiler compiler;
  init_compiler(p, &compiler, FnType_script);

  p->in_expr_stat = false;
  p->last_name_valid = false;
  p->errc = 0;
  p->panic = false;
  p->within_struct = false;

  advance(p);
  while (!consume(p, tok_eof)) {
    decl(p);
  }
  expect(p, tok_eof, err_msg_expect_eof);

  GcFn* fn = end_compiler(p);
  return p->errc > 0 ? NULL : fn;
}

void mark_compiler_roots(Parser* p) {
  if (p == NULL) {
    return;
  }

  Compiler* compiler = p->compiler;
  while (compiler != NULL) {
    mark_obj(p->h, (GcObj*)compiler->fn);
    compiler = compiler->enclosing;
  }

  mark_val(p->h, p->cur.val);
  mark_val(p->h, p->prev.val);
}
