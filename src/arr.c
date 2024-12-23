#include "arr.h"
#include "mem.h"

void ensure_len(hby_State* h, VArr* arr, int len) {
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

void push_varr(hby_State* h, VArr* arr, Val val) {
  ensure_len(h, arr, arr->len + 1);
  arr->items[arr->len++] = val;
}

void insert_varr(hby_State* h, VArr* arr, Val val, int index) {
  ensure_len(h, arr, arr->len + 1);
  arr->len++;
  for (int i = arr->len; i > index; i--) {
    arr->items[i] = arr->items[i - 1];
  }
  arr->items[index] = val;
}

void rem_varr(hby_State* h, VArr* arr, int index) {
  for (int i = index; i < arr->len - 1; i++) {
    arr->items[i] = arr->items[i + 1];
  }
  arr->len--;
}

void free_varr(hby_State* h, VArr* arr) {
  release_arr(h, Val, arr->items, arr->cap);
  init_varr(arr);
}
