#include "arr.h"
#include "hby.h"
#include "lib.h"
#include "table.h"
#include "mem.h"
#include "obj.h"
#include "tostr.h"
#include "val.h"
#include "state.h"

// TODO: Make all of this better. It sucks and doesn't use the C API well

static GcArr* self(hby_State* h) {
  return as_arr(*(h->frame->base));
}

static bool arr_len(hby_State* h, int argc) {
  hby_push_num(h, hby_len(h, 0));
  return true;
}

static bool arr_push(hby_State* h, int argc) {
  GcArr* arr = self(h);

  for (int i = 0; i < argc; i++) {
    push_varr(h, &arr->varr, *(h->frame->base + i + 1));
  }
  return false;
}

static bool arr_insert(hby_State* h, int argc) {
  int len = hby_len(h, 0);

  GcArr* arr = self(h);

  // For insert, you're allowed to pass in `arr.len()`, to put an element at the
  // end of the array. So we can't use `get_index()`, which disallows this.
  int index = hby_get_num(h, 1);
  if (index < 0) {
    index += len;
  }
  if (index < 0 || index > len) {
    hby_err(h, err_msg_index_out_of_bounds);
  }

  insert_varr(h, &arr->varr, *(h->frame->base + 2), index);
  return false;
}

static bool arr_rem(hby_State* h, int argc) {
  int len = hby_len(h, 0);

  GcArr* arr = self(h);
  int index = get_index(h, len, hby_get_num(h, 1));

  rem_varr(h, &arr->varr, index);
  return false;
}

static bool arr_swaprem(hby_State* h, int argc) {
  int len = hby_len(h, 0);

  GcArr* arr = self(h);
  int index = get_index(h, len, hby_get_num(h, 1));

  VArr* varr = &arr->varr;
  varr->items[index] = varr->items[varr->len - 1];
  varr->len--;
  return false;
}

static int find(VArr* arr, Val val) {
  for (int i = 0; i < arr->len; i++) {
    if (vals_eql(arr->items[i], val)) {
      return i;
    }
  }
  return -1;
}

static bool arr_erase(hby_State* h, int argc) {
  GcArr* arr = self(h);
  int index = find(&arr->varr, *(h->frame->base + 1));
  if (index == -1) {
    hby_push_bool(h, false);
    return true;
  }

  rem_varr(h, &arr->varr, index);
  hby_push_bool(h, true);
  return true;
}

static bool arr_find(hby_State* h, int argc) {
  GcArr* arr = self(h);
  hby_push_num(h, find(&arr->varr, *(h->frame->base + 1)));
  return true;
}

static int find_next_pow(int i) {
  i--;
  i |= i >> 1;
  i |= i >> 2;
  i |= i >> 4;
  i |= i >> 8;
  i |= i >> 16;
  i++;

  return i;
}

static bool arr_join(hby_State* h, int argc) {
  GcArr* arr = self(h);

  int cap = 8;
  int len = 0;
  char* chars = allocate(h, char, cap);

  for (int i = 0; i < arr->varr.len; i++) {
    GcStr* str = to_str(h, arr->varr.items[i]);
    if (len + str->len > cap) {
      int pcap = cap;
      cap = find_next_pow(len + str->len);
      chars = grow_arr(h, char, chars, pcap, cap);
    }

    memcpy(chars + len, str->chars, str->len);
    len += str->len;
  }

  chars = grow_arr(h, char, chars, cap, len + 1);
  chars[len] = '\0';
  hby_push_lstr(h, chars, len);
  return true;
}

static bool arr_clear(hby_State* h, int argc) {
  GcArr* arr = self(h);
  // Free is equivalent to clearing in this case
  free_varr(h, &arr->varr);
  return false;
}

static bool arr_eqls(hby_State* h, int argc) {
  if (!hby_is_array(h, 1)) {
    hby_push_bool(h, false);
    return true;
  }

  int len = hby_len(h, 0);

  if (hby_len(h, 1) != len) {
    hby_push_bool(h, false);
    return true;
  }

  for (int i = 0; i < len; i++) {
    hby_array_get_index(h, 0, i);
    hby_array_get_index(h, 1, i);

    if (!vals_eql(h->top[-1], h->top[-2])) {
      hby_pop(h, 2);
      hby_push_bool(h, false);
      return true;
    }

    hby_pop(h, 2);
  }

  hby_push_bool(h, true);
  return true;
}

static bool arr_copy(hby_State* h, int argc) {
  hby_push_array(h);

  int len = hby_len(h, 0);
  for (int i = 0; i < len; i++) {
    hby_array_get_index(h, 0, i);
    hby_array_add(h, -2);
  }

  return true;
}


hby_StructMethod arr_methods[] = {
  {"len", arr_len, 0, hby_method},
  {"push", arr_push, -1, hby_method},
  {"insert", arr_insert, 2, hby_method},
  {"rem", arr_rem, 1, hby_method},
  {"swaprem", arr_swaprem, 1, hby_method},
  {"erase", arr_erase, 1, hby_method},
  {"find", arr_find, 1, hby_method},
  {"join", arr_join, 0, hby_method},
  {"clear", arr_clear, 0, hby_method},
  {"eqls", arr_eqls, 1, hby_method},
  {"copy", arr_copy, 0, hby_method},
  {NULL, NULL, 0, 0},
};

bool open_arr(hby_State* h, int argc) {
  hby_push_struct(h, "Array");
  h->array_struct = as_struct(*(h->top - 1));
  hby_struct_add_members(h, arr_methods, -1);
  hby_set_global(h, NULL, -1);

  return false;
}
