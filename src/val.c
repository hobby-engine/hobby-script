#include "val.h"

#include <stdio.h>
#include <string.h>
#include "mem.h"

void init_valarr(ValArr* arr) {
  arr->items = NULL;
  arr->cap = 0;
  arr->len = 0;
}

void push_valarr(hbs_State* h, ValArr* arr, Val val) {
  if (arr->cap < arr->len + 1) {
    int pcap = arr->cap;
    arr->cap = grow_cap(pcap);
    arr->items = grow_arr(h, Val, arr->items, pcap, arr->cap);
  }

  arr->items[arr->len++] = val;
}

void rem_valarr(struct hbs_State* h, ValArr* arr, int idx) {
  for (int i = idx; i < arr->len - 1; i++) {
    arr->items[i] = arr->items[i + 1];
  }
  arr->len--;
}

void free_valarr(hbs_State* h, ValArr* arr) {
  free_arr(h, Val, arr->items, arr->cap);
  init_valarr(arr);
}

bool vals_eql(Val a, Val b) {
#ifdef nan_boxing
  return a == b;
#else
  if (a.type != b.type) {
    return false;
  }

  switch (a.type) {
    case val_bool: return as_bool(a) == as_bool(b);
    case val_null: return true;
    case val_num: return as_num(a) == as_num(b);
    case val_obj: return as_obj(a) == as_obj(b);
  }

  return false;
#endif
}
