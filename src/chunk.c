#include "chunk.h"

#include <stdlib.h>
#include "val.h"
#include "mem.h"

void init_chunk(Chunk* c) {
  c->len = 0;
  c->cap = 0;
  c->code = NULL;
  c->lines = NULL;

  init_valarr(&c->consts);
}

void free_chunk(hbs_State* h, Chunk* c) {
  free_arr(h, u8, c->code, c->cap);
  free_arr(h, int, c->lines, c->cap);
  free_valarr(h, &c->consts);
  init_chunk(c);
}

void write_chunk(hbs_State* h, Chunk* c, Bc bc, int line) {
  if (c->cap < c->len + 1) {
    int old_cap = c->cap;
    c->cap = grow_cap(old_cap);
    c->code = grow_arr(h, u8, c->code, old_cap, c->cap);
    c->lines = grow_arr(h, int, c->lines, old_cap, c->cap);
  }

  c->code[c->len] = bc;
  c->lines[c->len] = line;
  c->len++;
}

int add_const_chunk(hbs_State* h, Chunk* c, Val val) {
  push(h, val);
  push_valarr(h, &c->consts, val);
  pop(h);
  return c->consts.len - 1;
}
