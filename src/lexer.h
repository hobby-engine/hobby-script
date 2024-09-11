#ifndef __HBS_TOKENIZER_H
#define __HBS_TOKENIZER_H

#include "state.h"

typedef enum {
  tok_lparen, tok_rparen, // ( )
  tok_lbrace, tok_rbrace, // { }
  tok_lbracket, tok_rbracket, // [ ]

  tok_comma, tok_dot, tok_minus, // , . -
  tok_plus, tok_star, tok_slash, // + * /
  tok_semicolon, tok_bang, tok_lt, // ; ! <
  tok_gt, tok_eql, tok_colon, // > = :
  tok_percent, // %

  tok_plus_eql, tok_minus_eql, tok_star_eql,
  tok_slash_eql, tok_percent_eql,

  tok_eql_eql, tok_bang_eql, tok_gte, // == != >=
  tok_lte, tok_and, tok_or, // <= && ||
  tok_rarrow, tok_dot_dot, // => ..
  tok_plus_plus, tok_minus_minus, // ++ --

  tok_ident, tok_str, tok_num, // _ "_" 0-9
  tok_true, tok_false, tok_null, // true false null

  tok_struct, tok_if, tok_else, // struct if else
  tok_while, tok_for, tok_loop, // while for loop
  tok_fn, tok_enum, tok_self, // fn enum self
  tok_return, tok_var, tok_const, // return var const
  tok_global, tok_switch, tok_case, // global switch case
  tok_break, tok_continue, tok_static, // break continue

  tok_eof, tok_err,
} TokType;

typedef struct {
  TokType type;
  const char* start;
  int len;
  int line;
  Val val;
} Tok;

typedef struct Lexer {
  hbs_State* h;
  const char* start;
  const char* cur;
  int line;
} Lexer;

void init_lexer(hbs_State* h, Lexer* l, const char* src);
Tok next_token(Lexer* l);

#endif // __HBS_TOKENIZER_H
