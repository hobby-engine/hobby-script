#ifndef __HBY_STATE_H
#define __HBY_STATE_H

#include <setjmp.h>
#include "hby.h"
#include "map.h"
#include "val.h"

#define frames_max 64
#define stack_size (frames_max * uint8_count)

typedef enum {
  call_type_c, // This is a C function
  call_type_capi, // This is called from the C API
  call_type_hby, // This is called from other Hobby code
} CallType;

typedef struct {
  union {
    struct GcClosure* hby;
    struct GcCFn* c;
  } fn;
  u8* ip;
  Val* base;
  CallType type;
} CallFrame;

typedef struct LongJmp {
  struct LongJmp* prev;
  jmp_buf buf;
} LongJmp;

typedef struct {
  bool can_gc;
  size_t alloced;
  size_t next_gc;
  struct GcObj* objs;
  struct GcObj* udata;
  int grayc;
  int gray_cap;
  struct GcObj** gray_stack;
} GcState;

struct hby_State {
  CallFrame frame_stack[frames_max];
  CallFrame* frame;

  Val stack[stack_size];
  Val* top;

  Map globals;
  Map global_consts;
  Map strs;
  Map files;
  struct GcUpval* open_upvals;
  struct GcArr* args;

  struct GcStruct* number_struct;
  struct GcStruct* boolean_struct;
  struct GcStruct* function_struct;
  struct GcStruct* string_struct;
  struct GcStruct* array_struct;
  struct GcStruct* udata_struct;

  GcState gc;
  LongJmp* err_jmp;

  struct Parser* parser;
};

bool file_imported(hby_State* h, const char* name);
void reset_stack(hby_State* h);

inline void push(hby_State* h, Val val) {
  *h->top = val;
  h->top++;
}

inline Val pop(hby_State* h) {
  return *(--h->top);
}

#endif // __HBY_STATE_H
