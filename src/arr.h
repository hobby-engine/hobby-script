#ifndef __HBY_ARR_H
#define __HBY_ARR_H

#include "common.h"
#include "errmsg.h"
#include "hby.h"
#include "val.h"

typedef struct {
  int cap;
  int len;
  Val* items;
  GcObj* obj;
} VArr;

void init_varr(VArr* arr);
void push_varr(hby_State* h, VArr* arr, Val val);
void insert_varr(hby_State* h, VArr* arr, Val val, int index);
void rem_varr(hby_State* h, VArr* arr, int index);
void clear_varr(hby_State* h, VArr* arr);
void free_varr(hby_State* h, VArr* arr);

inline int get_index(hby_State* h, int len, int given) {
  if (given < 0) {
    given += len;
  }
  if (given < 0 || given >= len) {
    hby_err(h, err_msg_index_out_of_bounds);
  }
  return given;
}

#endif // __HBY_ARR_H
