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


bool open_arr(hbs_State* h, int argc) {
  Map* arr_methods = &h->arr_methods;

  {
    hbs_push_c_fn(h, "len", arr_len, 0);
    GcStr* name = copy_str(h, "len", 3);
    push(h, create_obj(name));
    set_map(h, arr_methods, name, *(h->top - 2));
    hbs_pop(h, 2); // name and c fn
  }

  {
    hbs_push_c_fn(h, "push", arr_push, 1);
    GcStr* name = copy_str(h, "push", 4);
    push(h, create_obj(name));
    set_map(h, arr_methods, name, *(h->top - 2));
    hbs_pop(h, 2); // name and c fn
  }

  {
    hbs_push_c_fn(h, "rem", arr_rem, 1);
    GcStr* name = copy_str(h, "rem", 3);
    push(h, create_obj(name));
    set_map(h, arr_methods, name, *(h->top - 2));
    hbs_pop(h, 2); // name and c fn
  }

  {
    hbs_push_c_fn(h, "swaprem", arr_swaprem, 1);
    GcStr* name = copy_str(h, "swaprem", 7);
    push(h, create_obj(name));
    set_map(h, arr_methods, name, *(h->top - 2));
    hbs_pop(h, 2); // name and c fn
  }

  return false;
}
