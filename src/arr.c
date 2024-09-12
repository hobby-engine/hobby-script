#include "arr.h"
#include "mem.h"

void ensure_len(hbs_State* h, VArr* arr, int len) {
  if (arr->cap < len) {
    int pcap = arr->cap;
    arr->cap = grow_cap(pcap);
    arr->items = grow_arr(h, Val, arr->items, pcap, arr->cap);
  }
}

void init_varr(VArr* arr) {
  arr->items = NULL;
  arr->cap = 0;
  arr->len = 0;
}

void push_varr(hbs_State* h, VArr* arr, Val val) {
  ensure_len(h, arr, arr->len + 1);
  arr->items[arr->len++] = val;
}

void insert_varr(hbs_State* h, VArr* arr, Val val, int idx) {
  ensure_len(h, arr, arr->len + 1);
  arr->len++;
  for (int i = arr->len; i > idx; i--) {
    arr->items[i] = arr->items[i - 1];
  }
  arr->items[idx] = val;
}

void rem_varr(hbs_State* h, VArr* arr, int idx) {
  for (int i = idx; i < arr->len - 1; i++) {
    arr->items[i] = arr->items[i + 1];
  }
  arr->len--;
}

void free_varr(hbs_State* h, VArr* arr) {
  release_arr(h, Val, arr->items, arr->cap);
  init_varr(arr);
}
