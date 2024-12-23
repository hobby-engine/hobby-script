#ifndef __HBY_TOKENIZER_H
#define __HBY_TOKENIZER_H

#include "hby.h"
#include "val.h"

#define max_strfmt_braces 4

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
  tok_rarrow, tok_dot_dot, tok_dot3, // => .. ...
  tok_plus_plus, tok_minus_minus, // ++ --

  tok_str, tok_strfmt, tok_strfmt_end,
  tok_ident, tok_num, 
  tok_true, tok_false, tok_null, // true false null

  tok_struct, tok_if, tok_else, // struct if else
  tok_while, tok_for, tok_loop, // while for loop
  tok_fn, tok_enum, tok_self, // fn enum self
  tok_return, tok_var, tok_const, // return var const
  tok_switch, tok_case, tok_break, // switch case break
  tok_continue, tok_static, tok_is, // continue static is
  tok_unreachable, // unreachable

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
  hby_State* h;
  const char* start;
  const char* cur;
  int line;
  char brace_terms[max_strfmt_braces];
  int brace_depth;
} Lexer;

void init_lexer(hby_State* h, Lexer* l, const char* src);
Tok next_token(Lexer* l);

#endif // __HBY_TOKENIZER_H
