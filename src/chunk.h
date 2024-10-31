#ifndef __HBY_CHUNK_H
#define __HBY_CHUNK_H

#include "common.h"
#include "val.h"
#include "arr.h"
#include "hby.h"

typedef enum {
  bc_pop,
  bc_def_global,
  bc_get_global,
  bc_set_global,
  bc_get_local,
  bc_set_local,
  bc_get_upval,
  bc_set_upval,
  bc_close_upval,
  bc_push_prop,
  bc_get_prop,
  bc_set_prop,
  bc_get_subscript,
  bc_set_subscript,
  bc_push_subscript,
  bc_destruct_array,
  bc_get_static,
  bc_init_prop,
  bc_const,
  bc_null,
  bc_true,
  bc_false,
  bc_array,
  bc_array_item,
  bc_add,
  bc_sub,
  bc_mul,
  bc_div,
  bc_mod,
  bc_eql,
  bc_neql,
  bc_gt,
  bc_lt,
  bc_gte,
  bc_lte,
  bc_cat,
  bc_is,
  bc_neg,
  bc_not,
  bc_ineq_jmp,
  bc_false_jmp,
  bc_jmp,
  bc_loop,
  bc_call,
  bc_invoke,
  bc_closure,
  bc_ret,
  bc_struct,
  bc_method,
  bc_def_static,
  bc_member,
  bc_enum,
  bc_inst,
  // Not used in final bytecode
  bc_break,
} Bc;

typedef struct {
  int len;
  int cap;
  u8* code;
  int* lines;
  VArr consts;
} Chunk;

void init_chunk(Chunk* c);
void free_chunk(hby_State* h, Chunk* c);
void write_chunk(hby_State* h, Chunk* c, Bc bc, int line);
int add_const_chunk(hby_State* h, Chunk* c, Val val);

#endif // __HBY_CHUNK_H
