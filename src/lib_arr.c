#include "arr.h"
#include "hbs.h"
#include "lib.h"
#include "map.h"
#include "obj.h"
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


static void add_method(hbs_State* h, const char* name, hbs_CFn fn, int argc) {
  hbs_push_cfunction(h, name, fn, argc);
  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));
  set_map(h, &h->arr_methods, str_name, *(h->top - 2));
  hbs_pop(h, 2); // name and c fn
}

hbs_CFnArgs arr_methods[] = {
  {"len", arr_len, 0},
  {"push", arr_push, -1},
  {"insert", arr_insert, 2},
  {"rem", arr_rem, 1},
  {"swaprem", arr_swaprem, 1},
  {"erase", arr_erase, 1},
  {"find", arr_find, 1},
  {NULL, NULL, 0},
};

bool open_arr(hbs_State* h, int argc) {
  for (hbs_CFnArgs* arg = arr_methods; arg->name != NULL; arg++) {
    add_method(h, arg->name, arg->fn, arg->argc);
  }
  return false;
}
