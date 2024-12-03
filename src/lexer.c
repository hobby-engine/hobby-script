#include "lexer.h"

#include <string.h>
#include <stdio.h>
#include "errmsg.h"
#include "mem.h"
#include "obj.h"

void init_lexer(hby_State* h, Lexer* l, const char* src) {
  if (strncmp(src, "\xEF\xBB\xBF", 3) == 0) {
    src += 3;
  }

  l->h = h;
  l->start = src;
  l->cur = src;
  l->line = 1;
  l->brace_depth = 0;
}

static bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z')
      || (c >= 'A' && c <= 'Z')
      || c == '_';
}

static bool is_at_end(Lexer* l) {
  return *l->cur == '\0';
}

static char advance(Lexer* l) {
  l->cur++;
  return l->cur[-1];
}

static char peek(Lexer* l) {
  return *l->cur;
}

static char peek_next(Lexer* l) {
  if (is_at_end(l)) {
    return '\0';
  }
  return l->cur[1];
}

static bool match(Lexer* l, char c) {
  if (is_at_end(l)) {
    return false;
  }
  if (*l->cur != c) {
    return false;
  }
  l->cur++;
  return true;
}

static Tok create_tok(Lexer* l, TokType type) {
  Tok t;
  t.type = type;
  t.start = l->start;
  t.len = (int)(l->cur - l->start);
  t.line = l->line;
  t.val = create_null();
  return t;
}

static Tok create_err_tok(Lexer* l, const char* msg) {
  Tok t;
  t.type = tok_err;
  t.start = msg;
  t.len = (int)strlen(msg);
  t.line = l->line;
  t.val = create_null();
  return t;
}

static void skip_whitespace(Lexer* l) {
  while (true) {
    char c = peek(l);
    switch (c) {
      case '\n':
        l->line++;
        advance(l);
        break;
      case ' ':
      case '\r':
      case '\t':
        advance(l);
        break;
      case '/':
        if (peek_next(l) == '/') {
          while (peek(l) != '\n' && !is_at_end(l)) {
            advance(l);
          }
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static Tok str(Lexer* l, bool is_fmt, bool closing_fmt) {
  TokType type = closing_fmt ? tok_strfmt_end : tok_str;
  int cap = 8;
  int len = 0;
  char* chars = allocate(l->h, char, cap);

  bool unterminated = false;

  while (peek(l) != l->brace_terms[l->brace_depth] && !is_at_end(l)) {
    if (len + 1 > cap) {
      int pcap = cap;
      cap = grow_cap(cap);
      chars = grow_arr(l->h, char, chars, pcap, cap);
    }

    char c = peek(l);
    if (c == '\n') {
      unterminated = true;
    }

    if (is_fmt && c == '{') {
      // if (l->brace_depth >= max_strfmt_braces) {
      //   release_arr(l->h, char, chars, cap);
      //   return create_err_tok(l, "Too many nested formatted strings");
      // }
      
      advance(l);
      l->brace_depth++;
      type = tok_strfmt;
      goto fmtd_str;
    }

    if (c == '\\') {
      advance(l);
      c = peek(l);

      switch (c) {
        case '\n':
        case 'n':  c = '\n'; break;
        case 't':  c = '\t'; break;
        case 'r':  c = '\r'; break;
        case 'a':  c = '\a'; break;
        case '\'':
        case '"':
        case '\\':
        case '{':
          break;
        default:
          release_arr(l->h, char, chars, cap);
          return create_err_tok(l, err_msg_escape_str);
      }
    }

    chars[len++] = c;

    advance(l);
  }

  if (unterminated || is_at_end(l)) {
    release_arr(l->h, char, chars, cap);
    return create_err_tok(l, err_msg_eof_str);
  }

  advance(l); // Get the closing quote

fmtd_str: {
    Tok str_tok;
    str_tok.type = type;
    str_tok.start = l->start;
    str_tok.len = (int)(l->cur - l->start);
    str_tok.line = l->line;
    str_tok.val = create_obj(copy_str(l->h, chars, len));

    release_arr(l->h, char, chars, cap);
    return str_tok;
  }
}

static Tok num(Lexer* l) {
  while (is_digit(peek(l))) {
    advance(l);
  }

  if (peek(l) == '.') {
    advance(l); // .
    while (is_digit(peek(l))) {
      advance(l);
    }
  }

  return create_tok(l, tok_num);
}

static TokType check_keyword(
    Lexer* l, int start, int len, const char* rest, TokType type) {
  if (l->cur - l->start == start + len 
      && memcmp(l->start + start, rest, len) == 0) {
    return type;
  }
  return tok_ident;
}

static TokType get_keyword(Lexer* l) {
  switch (*l->start) {
    case 'b': return check_keyword(l, 1, 4, "reak", tok_break);
    case 'c': 
      if (l->cur - l->start > 1) {
        switch (l->start[1]) {
          case 'a': return check_keyword(l, 2, 2, "se", tok_case);
          case 'o': {
            if (check_keyword(l, 2, 3, "nst", tok_null) != tok_ident) {
              return tok_const;
            } else if (check_keyword(l, 2, 6, "ntinue", tok_null) != tok_ident) {
              return tok_continue;
            } else {
              return tok_ident;
            }
          }
        }
      }
      break;
    case 'e':
      if (l->cur - l->start > 1) {
        switch (l->start[1]) {
          case 'l': return check_keyword(l, 2, 2, "se", tok_else);
          case 'n': return check_keyword(l, 2, 2, "um", tok_enum);
        }
      }
      break;
    case 'f':
      if (l->cur - l->start > 1) {
        switch (l->start[1]) {
          case 'a': return check_keyword(l, 2, 3, "lse", tok_false);
          case 'o': return check_keyword(l, 2, 1, "r", tok_for);
          case 'n': return check_keyword(l, 2, 0, "", tok_fn);
        }
      }
      break;
    case 'i':
      if (l->cur - l->start > 1) {
        switch (l->start[1]) {
          case 'f': return check_keyword(l, 2, 0, "", tok_if);
          case 's': return check_keyword(l, 2, 0, "", tok_is);
        }
      }
      break;
    case 'l': return check_keyword(l, 1, 3, "oop", tok_loop);
    case 'n': return check_keyword(l, 1, 3, "ull", tok_null);
    case 'r': return check_keyword(l, 1, 5, "eturn", tok_return);
    case 's': 
      if (l->cur - l->start > 1) {
        switch (l->start[1]) {
          case 'e': return check_keyword(l, 2, 2, "lf", tok_self);
          case 't':
            if (l->cur - l->start > 2) {
              switch (l->start[2]) {
                case 'a': return check_keyword(l, 3, 3, "tic", tok_static);
                case 'r': return check_keyword(l, 3, 3, "uct", tok_struct);
              }
            }
            break;
          case 'w': return check_keyword(l, 2, 4, "itch", tok_switch);
        }
      }
      break;
    case 't': return check_keyword(l, 1, 3, "rue", tok_true);
    case 'u': return check_keyword(l, 1, 10, "nreachable", tok_unreachable);
    case 'v': return check_keyword(l, 1, 2, "ar", tok_var);
    case 'w': return check_keyword(l, 1, 4, "hile", tok_while);
  }

  return tok_ident;
}

static Tok ident(Lexer* l) {
  while (is_alpha(peek(l)) || is_digit(peek(l))) {
    advance(l);
  }
  return create_tok(l, get_keyword(l));
}

Tok next_token(Lexer* l) {

  skip_whitespace(l);
  l->start = l->cur;

  if (is_at_end(l)) {
    return create_tok(l, tok_eof);
  }

  char c = advance(l);

  if (is_digit(c)) {
    return num(l);
  }
  if (is_alpha(c)) {
    return ident(l);
  }

  switch (c) {
    case '(': return create_tok(l, tok_lparen);
    case ')': return create_tok(l, tok_rparen);
    case '{': return create_tok(l, tok_lbrace);
    case '}': {
      if (l->brace_depth > 0) {
        l->brace_depth--;
        return str(l, true, true);
      }
      return create_tok(l, tok_rbrace);
    }
    case '[': return create_tok(l, tok_lbracket);
    case ']': return create_tok(l, tok_rbracket);
    case ',': return create_tok(l, tok_comma);
    case '.':
      if (is_digit(peek(l))) {
        return num(l);
      }

      if (match(l, '.')) {
        if (match(l, '.')) {
          return create_tok(l, tok_dot3);
        }
        return create_tok(l, tok_dot_dot);
      }
      return create_tok(l, tok_dot);
    case '+':
      if (match(l, '+')) {
        return create_tok(l, tok_plus_plus);
      }
      return create_tok(l, match(l, '=') ? tok_plus_eql : tok_plus);
    case '-':
      if (match(l, '>')) {
        return create_tok(l, tok_rarrow);
      }
      if (match(l, '-')) {
        return create_tok(l, tok_minus_minus);
      }
      return create_tok(l, match(l, '=') ? tok_minus_eql : tok_minus);
    case '*':
      return create_tok(l, match(l, '=') ? tok_star_eql : tok_star);
    case '/':
      return create_tok(l, match(l, '=') ? tok_slash_eql : tok_slash);
    case '%':
      return create_tok(l, match(l, '=') ? tok_percent_eql : tok_percent);
    case ';': return create_tok(l, tok_semicolon);
    case ':': return create_tok(l, tok_colon);
    case '=': {
      return create_tok(l, match(l, '=') ? tok_eql_eql : tok_eql);
    }
    case '!':
      return create_tok(l, match(l, '=') ? tok_bang_eql : tok_bang);
    case '>': return create_tok(l, match(l, '=') ? tok_gte : tok_gt);
    case '<': return create_tok(l, match(l, '=') ? tok_lte : tok_lt);
    case '&':
      return match(l, '&')
        ? create_tok(l, tok_and)
        : create_err_tok(l, "Bitwise and operator not supported ('&')");
    case '|':
      return match(l, '|')
        ? create_tok(l, tok_or)
        : create_err_tok(l, "Bitwise or operator not supported ('|')");
    case '\'':
    case '"':
      if (l->brace_depth >= max_strfmt_braces) {
        return create_err_tok(l, err_msg_max_strfmt);
      }
      l->brace_terms[l->brace_depth] = c;
      return str(l, false, false);
    case '$': {
      if (l->brace_depth >= max_strfmt_braces) {
        return create_err_tok(l, err_msg_max_strfmt);
      }
      char term = advance(l);
      l->brace_terms[l->brace_depth] = term;
      return str(l, true, false);
    }
  }

  return create_err_tok(l, err_msg_invalid_char);
}
