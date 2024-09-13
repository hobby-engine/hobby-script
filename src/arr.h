#ifndef __HBS_ARR_H
#define __HBS_ARR_H

#include "common.h"
#include "errmsg.h"
#include "hbs.h"
#include "val.h"

typedef struct {
  int cap;
  int len;
  Val* items;
  GcObj* obj;
} VArr;

void init_varr(VArr* arr);
void push_varr(struct hbs_State* h, VArr* arr, Val val);
void insert_varr(struct hbs_State* h, VArr* arr, Val val, int idx);
void rem_varr(struct hbs_State* h, VArr* arr, int idx);
void free_varr(struct hbs_State* h, VArr* arr);

inline int get_idx(hbs_State* h, int len, int given) {
  if (given < 0) {
    given += len;
  }
  if (given < 0 || given >= len) {
    hbs_err(h, err_msg_index_out_of_bounds);
  }
  return given;
}

#endif // __HBS_ARR_H
