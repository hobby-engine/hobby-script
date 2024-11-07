#include "chunk.h"

#include <stdlib.h>
#include "val.h"
#include "mem.h"

void init_chunk(Chunk* c) {
  c->len = 0;
  c->cap = 0;
  c->code = NULL;
  c->lines = NULL;

  init_varr(&c->consts);
}

void free_chunk(hby_State* h, Chunk* c) {
  release_arr(h, uint8_t, c->code, c->cap);
  release_arr(h, int, c->lines, c->cap);
  free_varr(h, &c->consts);
  init_chunk(c);
}

void write_chunk(hby_State* h, Chunk* c, Bc bc, int line) {
  if (c->cap < c->len + 1) {
    int old_cap = c->cap;
    c->cap = grow_cap(old_cap);
    c->code = grow_arr(h, uint8_t, c->code, old_cap, c->cap);
    c->lines = grow_arr(h, int, c->lines, old_cap, c->cap);
  }

  c->code[c->len] = bc;
  c->lines[c->len] = line;
  c->len++;
}

int add_const_chunk(hby_State* h, Chunk* c, Val val) {
  push(h, val);
  push_varr(h, &c->consts, val);
  pop(h);
  return c->consts.len - 1;
}
