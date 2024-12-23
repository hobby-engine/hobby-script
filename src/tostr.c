#include "tostr.h"

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "obj.h"
#include "mem.h"

#define num_fmt "%.14g"

static GcStr* fn_to_str(hby_State* h, GcFn* fn) {
  if (fn->name == NULL) {
    return copy_str(h, "<script>", 8);
  }
  return str_fmt(h, "<fn @>", fn->name);
}

static GcStr* arr_to_str(hby_State* h, GcArr* arr, int depth) {
  if (depth > 2) {
    return copy_str(h, "[...]", 5);
  }
  int cap = 8;
  int len = 0;
  char* chars = allocate(h, char, cap);

  chars[len++] = '[';
  for (int i = 0; i < arr->varr.len; i++) {
    bool add_quotes = false;
    GcStr* str = NULL;
    if (is_arr(arr->varr.items[i])) {
      str = arr_to_str(h, as_arr(arr->varr.items[i]), depth + 1);
    } else {
      add_quotes = is_str(arr->varr.items[i]);
      str = to_str(h, arr->varr.items[i]);
    }
    push(h, create_obj(str));

    int add_len = str->len + 2; // + 2 is for comma and space
    if (add_quotes) {
      add_len += 2;
    }
    if (len + add_len > cap) {
      int old_cap = cap;
      while (len + add_len > cap) {
        cap = grow_cap(cap);
      }
      chars = grow_arr(h, char, chars, old_cap, cap);
    }

    pop(h);

    if (add_quotes) {
      chars[len++] = '"';
    }

    memcpy(chars + len, str->chars, str->len);
    len += str->len;

    if (add_quotes) {
      chars[len++] = '"';
    }

    if (i != arr->varr.len - 1) {
      chars[len++] = ',';
      chars[len++] = ' ';
    }
  }

  // We know the string has enough space left in it because we added 2 for the
  // comma and space after elements, which isn't present on the last element
  chars[len++] = ']';

  GcStr* copy = copy_str(h, chars, len);
  release_arr(h, char, chars, cap);
  return copy;
}


GcStr* to_str(hby_State* h, Val val) {
  if (is_str(val)) {
    return as_str(val);
  } else if (is_num(val)) {
    return num_to_str(h, as_num(val));
  } else if (is_bool(val)) {
    return bool_to_str(h, as_bool(val));
  } else if (is_null(val)) {
    return copy_str(h, "null", 4);
  } else if (is_arr(val)) {
    return arr_to_str(h, as_arr(val), 1);
  } else if (is_udata(val)) {
    return copy_str(h, "<userdata>", 10);
  } else if (is_map(val)) {
    return copy_str(h, "<map>", 5);
  } else if (is_fn(val)) {
    return fn_to_str(h, as_fn(val));
  } else if (is_closure(val)) {
    return fn_to_str(h, as_closure(val)->fn);
  } else if (is_method(val)) {
    GcMethod* m = as_method(val);
    switch (anyfn_type(m->fn)) {
      case obj_closure:
        return fn_to_str(h, m->fn.hby->fn);
      case obj_c_fn:
        return str_fmt(h, "<c fn @>", m->fn.c->name);
      default:
        ; // unreachable
    }
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

GcStr* num_to_str(hby_State* h, double n) {
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

GcStr* bool_to_str(hby_State* h, bool b) {
  return b ? copy_str(h, "true", 4) : copy_str(h, "false", 5);
}

GcStr* str_fmt(hby_State* h, const char* fmt, ...) {
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
