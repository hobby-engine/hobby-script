#ifndef __HBS_STATE_H
#define __HBS_STATE_H

#include <setjmp.h>
#include "hbs.h"
#include "map.h"
#include "val.h"

#define frames_max 64
#define stack_size (frames_max * uint8_count)

typedef enum {
  call_type_c, // This is a C function
  call_type_script, // This is a script
  call_type_reenter, // This is called from the C API
  call_type_hbs, // This is called from other Hobby code
} CallType;

typedef struct {
  union {
    struct GcClosure* hbs;
    struct GcCFn* c;
  } fn;
  u8* ip;
  Val* base;
  CallType type;
} CallFrame;

struct hbs_State {
  CallFrame frame_stack[frames_max];
  CallFrame* frame;

  Val stack[stack_size];
  Val* top;

  Map globals;
  Map strs;
  Map files;
  struct GcUpval* open_upvals;
  struct GcObj* objs;

  struct GcStruct* number_struct;
  struct GcStruct* boolean_struct;
  struct GcStruct* function_struct;
  struct GcStruct* string_struct;
  struct GcStruct* array_struct;

  bool can_gc;
  size_t alloced;
  size_t next_gc;

  jmp_buf err_jmp;

  int grayc;
  int gray_cap;
  struct GcObj** gray_stack;

  struct Parser* parser;
};

bool file_imported(hbs_State* h, const char* name);
void reset_stack(hbs_State* h);

inline void push(hbs_State* h, Val val) {
  *h->top = val;
  h->top++;
}

inline Val pop(hbs_State* h) {
  return *(--h->top);
}

#endif // __HBS_STATE_H
