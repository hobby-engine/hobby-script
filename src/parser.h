#ifndef __HBY_PARSER_H
#define __HBY_PARSER_H

#include "obj.h"
#include "lexer.h"
#include "hby.h"

typedef struct Parser {
  Lexer lexer;
  Tok cur;
  Tok prev;
  bool panic;
  int errc;

  bool in_expr_stat;

  // Last name is used for identifying structs, enums, and methods.
  // the `last_name` is only valid for the duration of the statement it was set
  // in.
  bool last_name_valid;
  Tok last_name;

  bool within_struct;

  const char* file_path;
  struct Compiler* compiler;
  hby_State* h;
} Parser;

GcFn* compile_hby(hby_State* h, const char* path, const char* src);
void mark_compiler_roots(Parser* p);

#endif // __HBY_PARSER_H
