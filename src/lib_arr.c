#include "arr.h"
#include "hbs.h"
#include "lib.h"
#include "map.h"
#include "mem.h"
#include "obj.h"
#include "tostr.h"
#include "val.h"

// TODO: Make all of this better. It sucks and doesn't use the C API well

static GcArr* self(hbs_State* h) {
  return as_arr(*(h->frame->base));
}

static bool arr_len(hbs_State* h, int argc) {
  hbs_expect_array(h, 0);
  hbs_push_num(h, hbs_len(h, 0));
  return true;
}

static bool arr_push(hbs_State* h, int argc) {
  hbs_expect_array(h, 0);

  GcArr* arr = self(h);

  for (int i = 0; i < argc; i++) {
    push_varr(h, &arr->varr, *(h->frame->base + i + 1));
  }
  return false;
}

static bool arr_insert(hbs_State* h, int argc) {
  hbs_expect_array(h, 0);
  int len = hbs_len(h, 0);

  GcArr* arr = self(h);

  // For insert, you're allowed to pass in `arr.len()`, to put an element at the
  // end of the array. So we can't use `get_idx()`, which disallows this.
  int idx = hbs_get_num(h, 1);
  if (idx < 0) {
    idx += len;
  }
  if (idx < 0 || idx > len) {
    hbs_err(h, err_msg_index_out_of_bounds);
  }

  insert_varr(h, &arr->varr, *(h->frame->base + 2), idx);
  return false;
}

static bool arr_rem(hbs_State* h, int argc) {
  hbs_expect_array(h, 0);
  int len = hbs_len(h, 0);

  GcArr* arr = self(h);
  int idx = get_idx(h, len, hbs_get_num(h, 1));

  rem_varr(h, &arr->varr, idx);
  return false;
}

static bool arr_swaprem(hbs_State* h, int argc) {
  hbs_expect_array(h, 0);
  int len = hbs_len(h, 0);

  GcArr* arr = self(h);
  int idx = get_idx(h, len, hbs_get_num(h, 1));

  VArr* varr = &arr->varr;
  varr->items[idx] = varr->items[varr->len - 1];
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

static bool arr_erase(hbs_State* h, int argc) {
  hbs_expect_array(h, 0);
  GcArr* arr = self(h);
  int idx = find(&arr->varr, *(h->frame->base + 1));
  if (idx == -1) {
    hbs_push_bool(h, false);
    return true;
  }

  rem_varr(h, &arr->varr, idx);
  hbs_push_bool(h, true);
  return true;
}

static bool arr_find(hbs_State* h, int argc) {
  hbs_expect_array(h, 0);
  GcArr* arr = self(h);
  hbs_push_num(h, find(&arr->varr, *(h->frame->base + 1)));
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

static bool arr_join(hbs_State* h, int argc) {
  hbs_expect_array(h, 0);
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
  hbs_push_string(h, chars, len);
  return true;
}


hbs_StructMethod arr_methods[] = {
  {"len", arr_len, 0, hbs_method},
  {"push", arr_push, -1, hbs_method},
  {"insert", arr_insert, 2, hbs_method},
  {"rem", arr_rem, 1, hbs_method},
  {"swaprem", arr_swaprem, 1, hbs_method},
  {"erase", arr_erase, 1, hbs_method},
  {"find", arr_find, 1, hbs_method},
  {"join", arr_join, 0, hbs_method},
  {NULL, NULL, 0, 0},
};

bool open_arr(hbs_State* h, int argc) {
  hbs_push_struct(h, "Array");
  h->array_struct = as_struct(*(h->top - 1));
  hbs_add_members(h, arr_methods, -2);
  hbs_set_global(h, NULL);

  return false;
}
