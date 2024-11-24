#ifndef __HBY_STATE_H
#define __HBY_STATE_H

#include <setjmp.h>
#include "hby.h"
#include "table.h"
#include "val.h"
#include "obj.h"
#include "parser.h"

#define frames_max 64
#define stack_size (frames_max * uint8_count)

typedef enum {
  call_type_c, // This is a C function
  call_type_capi, // This is called from the C API
  call_type_hby, // This is called from other Hobby code
} CallType;

typedef struct {
  GcAnyFn fn;
  uint8_t* ip;
  Val* base;
  CallType type;
} CallFrame;

typedef struct PCall {
  struct PCall* prev;
  GcAnyFn callback;
  jmp_buf buf;
} PCall;

typedef struct {
  bool can_gc;
  size_t alloced;
  size_t next_gc;
  GcObj* objs;
  GcObj* udata;
  int grayc;
  int gray_cap;
  GcObj** gray_stack;
} GcState;

struct hby_State {
  CallFrame frame_stack[frames_max];
  CallFrame* frame;

  Val stack[stack_size];
  Val* top;

  Table globals;
  Table global_consts;
  Table strs;
  Table files;

  GcMap* registry;

  GcUpval* open_upvals;
  GcArr* args;

  GcStruct* number_struct;
  GcStruct* boolean_struct;
  GcStruct* function_struct;
  GcStruct* string_struct;
  GcStruct* array_struct;
  GcStruct* map_struct;
  GcStruct* udata_struct;

  GcState gc;
  PCall* pcall;

  Parser* parser;
};

void reset_stack(hby_State* h);

inline void push(hby_State* h, Val val) {
  *h->top = val;
  h->top++;
}

inline Val pop(hby_State* h) {
  return *(--h->top);
}

#endif // __HBY_STATE_H
