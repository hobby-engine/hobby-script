#include "errmsg.h"
#include "hby.h"
#include "lib.h"
#include "mem.h"
#include "obj.h"
#include "val.h"
#include <stdio.h>

static bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool str_len(hby_State* h, int argc) {
  hby_expect_string(h, 0);
  hby_push_num(h, hby_len(h, 0));
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
    hby_State* h,
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

static bool str_find(hby_State* h, int argc) {
  size_t substr_len;
  const char* substr = hby_get_tostring(h, 1, &substr_len);
  size_t str_len;
  const char* str = hby_get_string(h, 0, &str_len);

  hby_push_num(h, find_substr(str, str_len, substr, substr_len));
  return true;
}

static bool str_rem(hby_State* h, int argc) {
  size_t str_len;
  const char* str = hby_get_string(h, 0, &str_len);

  int start = get_idx(h, str_len, hby_get_num(h, 1));
  int len = hby_get_num(h, 2);
  if (len < 0) {
    len += str_len - start + 1;
  }
  int end = start + len;

  if (end < 0 || end > (int)str_len) {
    hby_err(h, err_msg_index_out_of_bounds);
  }

  const char* lhs = str;
  size_t lhs_len = start;

  const char* rhs = str + end;
  size_t rhs_len = str_len - end;

  size_t res_len;
  char* res = concat(h, lhs, lhs_len, rhs, rhs_len, &res_len);

  hby_push_lstr(h, res, res_len);
  return true;
}

static bool str_trim(hby_State* h, int argc) {
  size_t str_len;
  const char* str = hby_get_string(h, 0, &str_len);

  int start = 0;
  int end = str_len;

  for (size_t i = 0; i < str_len && is_whitespace(str[i]); i++) {
    start++;
  }

  if (start < end) {
    for (int i = str_len - 1; i >= 0 && is_whitespace(str[i]); i--) {
      end--;
    }
  }

  hby_push_lstrcpy(h, str + start, end - start);
  return true;
}

static bool str_split(hby_State* h, int argc) {
  size_t str_len;
  const char* str = hby_get_string(h, 0, &str_len);

  size_t delim_len;
  const char* delim = hby_get_string(h, 1, &delim_len);

  hby_push_array(h);

  int start = 0;
  int len = 0;

  while (start + len < (int)str_len) {
    int i = start + len;

    bool is_delim = true;

    for (size_t j = 0; j < delim_len; j++) {
      if (i + j >= str_len) {
        is_delim = false;
        break;
      }

      char d = delim[j];
      char c = str[i + j];

      if (c != d) {
        is_delim = false;
        break;
      }
    }

    if (is_delim) {
      if (len > 0) {
        hby_push_lstrcpy(h, str + start, len);
        hby_array_add(h, -2);
      }
      start += len + delim_len;
      len = 0;
    } else {
      len++;
    }
  }

  if (len > 0) {
    hby_push_lstrcpy(h, str + start, len);
    hby_array_add(h, -2);
  }

  return true;
}

static bool is_lower(char c) {
  return c >= 'a' && c <= 'z';
}

static bool is_upper(char c) {
  return c >= 'A' && c <= 'Z';
}

static bool str_toup(hby_State* h, int argc) {
  size_t len;
  const char* str = hby_get_string(h, 0, &len);

  char* upper = allocate(h, char, len + 1);
  upper[len] = '\0';

  for (size_t i = 0; i < len; i++) {
    char c = str[i];
    if (is_lower(c)) {
      c -= 'a' - 'A';
    }
    upper[i] = c;
  }

  hby_push_lstr(h, upper, len);
  return true;
}

static bool str_tolow(hby_State* h, int argc) {
  size_t len;
  const char* str = hby_get_string(h, 0, &len);

  char* upper = allocate(h, char, len + 1);
  upper[len] = '\0';

  for (size_t i = 0; i < len; i++) {
    char c = str[i];
    if (is_upper(c)) {
      c += 'a' - 'A';
    }
    upper[i] = c;
  }

  hby_push_lstr(h, upper, len);
  return true;
}

static bool str_isdigit(hby_State* h, int argc) {
  size_t str_len;
  const char* str = hby_get_string(h, 0, &str_len);

  if (str_len == 0) {
    hby_push_bool(h, false);
    return true;
  }

  for (size_t i = 0; i < str_len; i++) {
    char c = str[i];
    if (c < '0' || c > '9') {
      hby_push_bool(h, false);
      return true;
    }
  }

  hby_push_bool(h, true);
  return true;
}

static bool str_isalpha(hby_State* h, int argc) {
  size_t str_len;
  const char* str = hby_get_string(h, 0, &str_len);

  if (str_len == 0) {
    hby_push_bool(h, false);
    return true;
  }

  for (size_t i = 0; i < str_len; i++) {
    char c = str[i];
    if ((c < 'a' || c > 'z') &&
        (c < 'A' || c > 'Z')) {
      hby_push_bool(h, false);
      return true;
    }
  }

  hby_push_bool(h, true);
  return true;
}

static bool str_isalphanum(hby_State* h, int argc) {
  size_t str_len;
  const char* str = hby_get_string(h, 0, &str_len);

  if (str_len == 0) {
    hby_push_bool(h, false);
    return true;
  }

  for (size_t i = 0; i < str_len; i++) {
    char c = str[i];
    if ((c < 'a' || c > 'z') &&
        (c < 'A' || c > 'Z') &&
        (c < '0' || c > '9')) {
      hby_push_bool(h, false);
      return true;
    }
  }

  hby_push_bool(h, true);
  return true;
}


hby_StructMethod str_methods[] = {
  {"len", str_len, 0, hby_method},
  {"find", str_find, 1, hby_method},
  {"rem", str_rem, 2, hby_method},
  {"trim", str_trim, 0, hby_method},
  {"split", str_split, 1, hby_method},
  {"toup", str_toup, 0, hby_method},
  {"tolow", str_tolow, 0, hby_method},
  {"isdigit", str_isdigit, 0, hby_method},
  {"isalpha", str_isalpha, 0, hby_method},
  {"isalphanum", str_isalphanum, 0, hby_method},
  {NULL, NULL, 0, 0},
};

bool open_str(hby_State* h, int argc) {
  hby_push_struct(h, "String");
  h->string_struct = as_struct(*(h->top - 1));
  hby_struct_add_members(h, str_methods, -1);
  hby_set_global(h, NULL);

  return false;
}
