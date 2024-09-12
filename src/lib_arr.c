#include "errmsg.h"
#include "hbs.h"
#include "lib.h"
#include "map.h"
#include "val.h"
#include "obj.h"

// TODO: Make all of this better. It sucks and doesn't use the C API well

static bool arr_len(hbs_State* h, int argc) {
  Val val = *(h->frame->base);
  if (!is_arr(val)) {
    hbs_err(h, "Expected array");
  }

  hbs_push_num(h, as_arr(val)->arr.len);
  return true;
}

static bool arr_push(hbs_State* h, int argc) {
  Val val = *(h->frame->base);
  if (!is_arr(val)) {
    hbs_err(h, "Expected array");
  }

  push_valarr(h, &as_arr(val)->arr, *(h->frame->base + 1));
  return false;
}

static bool arr_rem(hbs_State* h, int argc) {
  Val val = *(h->frame->base);
  if (!is_arr(val)) {
    hbs_err(h, "Expected array");
  }

  GcArr* arr = as_arr(val);

  int idx = hbs_get_num(h, 1);
  if (idx < 0) {
    idx += arr->arr.len;
  }
  if (idx < 0 || idx > arr->arr.len) {
    hbs_err(h, err_msg_index_out_of_bounds);
  }

  rem_valarr(h, &arr->arr, idx);
  return false;
}

static bool arr_swaprem(hbs_State* h, int argc) {
  Val val = *(h->frame->base);
  if (!is_arr(val)) {
    hbs_err(h, "Expected array");
  }

  GcArr* arr = as_arr(val);

  int idx = hbs_get_num(h, 1);
  if (idx < 0) {
    idx += arr->arr.len;
  }
  if (idx < 0 || idx > arr->arr.len) {
    hbs_err(h, err_msg_index_out_of_bounds);
  }

  ValArr* varr = &arr->arr;
  varr->items[idx] = varr->items[varr->len - 1];
  varr->len--;
  return false;
}


static void add_method(hbs_State* h, const char* name, hbs_CFn fn, int argc) {
  hbs_push_cfunction(h, name, fn, argc);
  GcStr* str_name = copy_str(h, name, strlen(name));
  push(h, create_obj(str_name));
  set_map(h, &h->arr_methods, str_name, *(h->top - 2));
  hbs_pop(h, 2); // name and c fn
}

bool open_arr(hbs_State* h, int argc) {
  add_method(h, "len", arr_len, 0);
  add_method(h, "push", arr_push, 1);
  add_method(h, "rem", arr_rem, 1);
  add_method(h, "swaprem", arr_swaprem, 1);
  return false;
}
