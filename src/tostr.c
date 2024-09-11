#include "tostr.h"

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "obj.h"
#include "mem.h"

#define num_fmt "%.14g"

static GcStr* fn_to_str(hbs_State* h, GcFn* fn) {
  if (fn->name == NULL) {
    return copy_str(h, "<script>", 8);
  }
  return str_fmt(h, "<fn @>", fn->name);
}

GcStr* to_str(hbs_State* h, Val val) {
  if (is_str(val)) {
    return as_str(val);
  } else if (is_num(val)) {
    return num_to_str(h, as_num(val));
  } else if (is_bool(val)) {
    return bool_to_str(h, as_bool(val));
  } else if (is_null(val)) {
    return copy_str(h, "null", 4);
  } else if (is_arr(val)) {
    return copy_str(h, "<Array>", 7);
  } else if (is_fn(val)) {
    return fn_to_str(h, as_fn(val));
  } else if (is_closure(val)) {
    return fn_to_str(h, as_closure(val)->fn);
  } else if (is_method(val)) {
    return fn_to_str(h, as_method(val)->fn->fn);
  } else if (is_c_fn(val)) {
    return str_fmt(h, "<c fn @>", as_c_fn(val)->name);
  } else if (is_inst(val)) {
    return str_fmt(h, "<@ instance>", as_inst(val)->_struct->name);
  } else if (is_struct(val)) {
    return str_fmt(h, "<@>", as_struct(val)->name);
  } else if (is_enum(val)) {
    return str_fmt(h, "<@>", as_enum(val)->name);
  } else if (is_upval(val)) {
    return copy_str(h, "<upvalue>", 9);
  }

  return copy_str(h, "<unknown>", 9);
}

GcStr* num_to_str(hbs_State* h, double n) {
  if (isnan(n)) {
    return copy_str(h, "nan", 3);
  }

  if (isinf(n)) {
    if (n > 0) {
      return copy_str(h, "inf", 3);
    } else {
      return copy_str(h, "-inf", 4);
    }
  }

  int len = snprintf(NULL, 0, num_fmt, n);
  char* str = allocate(h, char, len + 1);
  snprintf(str, len + 1, num_fmt, n);
  return take_str(h, str, len);
}

GcStr* bool_to_str(hbs_State* h, bool b) {
  return b ? copy_str(h, "true", 4) : copy_str(h, "false", 5);
}

GcStr* str_fmt(hbs_State* h, const char* fmt, ...) {
  va_list args;

  va_start(args, fmt);
  size_t fmt_len = 0;

  for (const char* c = fmt; *c != '\0'; c++) {
    switch (*c) {
      case '$': fmt_len += strlen(va_arg(args, const char*)); break;
      case '@': {
        GcStr* s = va_arg(args, GcStr*);
        if (s != NULL) {
          fmt_len += s->len;
        } else {
          fmt_len += 4;
        }
        break;
      }
      default: fmt_len += 1;
    }
  }

  va_end(args);

  char* chars = allocate(h, char, fmt_len + 1);
  chars[fmt_len] = '\0';

  va_start(args, fmt);
  char* start = chars;
  for (const char* c = fmt; *c != '\0'; c++) {
    switch (*c) {
      case '$': {
        const char* s = va_arg(args, const char*);
        size_t len = strlen(s);
        memcpy(start, s, len);
        start += len;
        break;
      }
      case '@': {
        GcStr* s = va_arg(args, GcStr*);
        if (s != NULL) {
          memcpy(start, s->chars, s->len);
          start += s->len;
        } else {
          memcpy(start, "NULL", 4);
          start += 4;
        }
        break;
      }
      default: *start++ = *c; break;
    }
  }

  return take_str(h, chars, fmt_len);
}
