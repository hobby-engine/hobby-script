#ifndef __HBY_VAL_H
#define __HBY_VAL_H

#include "common.h"

typedef struct GcObj GcObj;
typedef struct GcStr GcStr;

#ifdef nan_boxing

#include <string.h>

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)

#define TAG_NULL  1
#define TAG_FALSE 2
#define TAG_TRUE  3

typedef uint64_t Val;

#define false_val       ((Val)(uint64_t)(QNAN | TAG_FALSE))
#define true_val        ((Val)(uint64_t)(QNAN | TAG_TRUE))

#define create_bool(b)  ((b) ? true_val : false_val)
#define create_null()   ((Val)(uint64_t)(QNAN | TAG_NULL))
#define create_num(num) num_to_val(num)
#define create_obj(obj) (Val)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

#define as_bool(v)      ((v) == true_val)
#define as_num(v)       val_to_num(v)
#define as_obj(v)       ((GcObj*)(uintptr_t)((v) & ~(SIGN_BIT | QNAN)))

#define is_obj(v)       (((v) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#define is_bool(v)      (((v) | 1) == true_val)
#define is_null(v)      ((v) == create_null())
#define is_num(v)       (((v) & QNAN) != QNAN)

static inline double val_to_num(Val val) {
  double num;
  memcpy(&num, &val, sizeof(Val));
  return num;
}

static inline Val num_to_val(double num) {
  Val val;
  memcpy(&val, &num, sizeof(double));
  return val;
}

#else

typedef enum {
  val_bool,
  val_null,
  val_num,
  val_obj,
} ValType;

typedef struct {
  ValType type;
  union {
    bool boolean;
    double num;
    GcObj* obj;
  } as;
} Val;

#define create_bool(v)   ((Val){val_bool, {.boolean=v}})
#define create_null()    ((Val){val_null, {.num=0}})
#define create_num(v)    ((Val){val_num,  {.num=v}})
#define create_obj(v)    ((Val){val_obj,  {.obj=(GcObj*)v}})

#define as_bool(v)       ((v).as.boolean)
#define as_num(v)        ((v).as.num)
#define as_obj(v)        ((v).as.obj)

#define is_bool(v)       ((v).type == val_bool)
#define is_null(v)       ((v).type == val_null)
#define is_num(v)        ((v).type == val_num)
#define is_obj(v)        ((v).type == val_obj)

#endif

bool vals_eql(Val a, Val b);

#endif // __HBY_VAL_H
