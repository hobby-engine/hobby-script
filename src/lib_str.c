#include "errmsg.h"
#include "hbs.h"
#include "lib.h"
#include "mem.h"
#include "obj.h"
#include "val.h"
#include <stdio.h>

static bool str_len(hbs_State* h, int argc) {
  hbs_expect_string(h, 0);
  hbs_push_num(h, hbs_len(h, 0));
  return true;
}

static int find_substr(
    const char* str, size_t str_len,
    const char* substr, size_t substr_len) {
  for (size_t i = 0; i < str_len - substr_len + 1; i++) {
    bool match = true;
    for (size_t j = 0; j < substr_len; j++) {
      char ssc = substr[j];
      char sc = str[i + j];
      if (ssc != sc) {
        match = false;
        break;
      }
    }

    if (match) {
      return i;
    }
  }

  return -1;
}

static char* concat(
    hbs_State* h,
    const char* lhs, size_t lhs_len,
    const char* rhs, size_t rhs_len,
    size_t* dest_len) {

  *dest_len = lhs_len + rhs_len;
  char* dest = allocate(h, char, *dest_len + 1);
  dest[*dest_len] = '\0';
  memcpy(dest, lhs, lhs_len);
  memcpy(dest + lhs_len, rhs, rhs_len);
  return dest;
}

static bool str_find(hbs_State* h, int argc) {
  size_t substr_len;
  const char* substr = hbs_get_tostring(h, 1, &substr_len);
  size_t str_len;
  const char* str = hbs_get_string(h, 0, &str_len);

  hbs_push_num(h, find_substr(str, str_len, substr, substr_len));
  return true;
}

static bool str_rem(hbs_State* h, int argc) {
  size_t str_len;
  const char* str = hbs_get_string(h, 0, &str_len);

  int start = get_idx(h, str_len, hbs_get_num(h, 1));
  int len = hbs_get_num(h, 2);
  if (len < 0) {
    len += str_len - start + 1;
  }
  int end = start + len;

  if (end < 0 || end > (int)str_len) {
    hbs_err(h, err_msg_index_out_of_bounds);
  }

  const char* lhs = str;
  size_t lhs_len = start;

  const char* rhs = str + end;
  size_t rhs_len = str_len - end;

  size_t res_len;
  char* res = concat(h, lhs, lhs_len, rhs, rhs_len, &res_len);

  hbs_push_string(h, res, res_len);
  return true;
}

static bool is_lower(char c) {
  return c >= 'a' && c <= 'z';
}

static bool is_upper(char c) {
  return c >= 'A' && c <= 'Z';
}

static bool str_toup(hbs_State* h, int argc) {
  size_t len;
  const char* str = hbs_get_string(h, 0, &len);

  char* upper = allocate(h, char, len + 1);
  upper[len] = '\0';

  for (size_t i = 0; i < len; i++) {
    char c = str[i];
    if (is_lower(c)) {
      c -= 'a' - 'A';
    }
    upper[i] = c;
  }

  hbs_push_string(h, upper, len);
  return true;
}

static bool str_tolow(hbs_State* h, int argc) {
  size_t len;
  const char* str = hbs_get_string(h, 0, &len);

  char* upper = allocate(h, char, len + 1);
  upper[len] = '\0';

  for (size_t i = 0; i < len; i++) {
    char c = str[i];
    if (is_upper(c)) {
      c += 'a' - 'A';
    }
    upper[i] = c;
  }

  hbs_push_string(h, upper, len);
  return true;
}

static bool str_isdigit(hbs_State* h, int argc) {
  size_t str_len;
  const char* str = hbs_get_string(h, 0, &str_len);

  if (str_len == 0) {
    hbs_push_bool(h, false);
    return true;
  }

  for (size_t i = 0; i < str_len; i++) {
    char c = str[i];
    if (c < '0' || c > '9') {
      hbs_push_bool(h, false);
      return true;
    }
  }

  hbs_push_bool(h, true);
  return true;
}


hbs_StructMethod str_methods[] = {
  {"len", str_len, 0, hbs_method},
  {"find", str_find, 1, hbs_method},
  {"rem", str_rem, 2, hbs_method},
  {"toup", str_toup, 0, hbs_method},
  {"tolow", str_tolow, 0, hbs_method},
  {"isdigit", str_isdigit, 0, hbs_method},
  {NULL, NULL, 0, 0},
};

bool open_str(hbs_State* h, int argc) {
  hbs_push_struct(h, "String");
  h->string_struct = as_struct(*(h->top - 1));
  hbs_add_members(h, str_methods, -2);
  hbs_set_global(h, NULL);

  return false;
}
