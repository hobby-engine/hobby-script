#include "state.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "tostr.h"
#include "compiler.h"
#include "mem.h"
#include "obj.h"
#include "hbs.h"
#include "lib.h"

hbs_State* create_state() {
  hbs_State* h = (hbs_State*)malloc(sizeof(hbs_State));

  // FIXME: The frame stack should be interfaced with the same as the normal
  // stack. This - 1 is hacky and shouldn't really exist.
  h->frame = h->frame_stack - 1;

  h->objs = NULL;
  h->parser = allocate(h, Parser, 1);
  h->parser->compiler = NULL;

  h->can_gc = true;
  h->alloced = 0;
  h->next_gc = 1024 * 1024;

  h->grayc = 0;
  h->gray_cap = 0;
  h->gray_stack = NULL;

  init_map(&h->globals);
  init_map(&h->strs);
  init_map(&h->files);
  init_map(&h->arr_methods);
  init_map(&h->str_methods);
  reset_stack(h);

  srand(time(NULL));

  // Using the C API like this is dangerous, since it cannot resolve errors.
  // TODO: Add a way to call API functions outside of the runtime
  // This all should be deferred until the VM starts.
  hbs_open_lib(h, open_arr);
  hbs_open_lib(h, open_core);
  hbs_open_lib(h, open_ease);
  hbs_open_lib(h, open_io);
  hbs_open_lib(h, open_math);
  hbs_open_lib(h, open_rand);
  hbs_open_lib(h, open_str);
  hbs_open_lib(h, open_sys);

  return h;
}

void free_state(hbs_State* h) {
  release(h, Parser, h->parser);
  free_map(h, &h->globals);
  free_map(h, &h->strs);
  free_map(h, &h->files);
  free_map(h, &h->arr_methods);
  free_map(h, &h->str_methods);
  free_objs(h);
  free(h);
}

bool file_imported(hbs_State* h, const char* name) {
  Val val;
  return get_map(&h->files, copy_str(h, name, strlen(name)), &val);
}

void reset_stack(hbs_State* h) {
  h->top = h->stack;
  h->frame = h->frame_stack;
  h->open_upvals = NULL;
}
