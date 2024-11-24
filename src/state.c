#include "state.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "tostr.h"
#include "parser.h"
#include "mem.h"
#include "obj.h"
#include "hby.h"
#include "lib.h"

hby_State* hby_create_state() {
  hby_State* h = (hby_State*)malloc(sizeof(hby_State));

  // FIXME: The frame stack should be interfaced with the same as the normal
  // stack. This - 1 is hacky and shouldn't really exist.
  h->frame = h->frame_stack - 1;

  h->parser = allocate(h, Parser, 1);
  h->parser->compiler = NULL;
  h->pcall = NULL;

  h->gc.objs = NULL;
  h->gc.udata = NULL;
  h->gc.can_gc = true;
  h->gc.alloced = 0;
  h->gc.next_gc = 1024 * 1024;
  h->gc.grayc = 0;
  h->gc.gray_cap = 0;
  h->gc.gray_stack = NULL;

  h->registry = create_map(h);
  h->args = create_arr(h);

  init_table(&h->globals);
  init_table(&h->global_consts);
  init_table(&h->strs);
  init_table(&h->files);
  reset_stack(h);

  srand(time(NULL));

  // Using the C API like this is dangerous, since it cannot resolve errors.
  // TODO: Add a way to call API functions outside of the runtime
  // This all should be deferred until the VM starts.
  hby_open_lib(h, open_arr);
  hby_open_lib(h, open_map);
  hby_open_lib(h, open_core);
  hby_open_lib(h, open_ease);
  hby_open_lib(h, open_io);
  hby_open_lib(h, open_math);
  hby_open_lib(h, open_rng);
  hby_open_lib(h, open_str);
  hby_open_lib(h, open_sys);

  return h;
}

void hby_free_state(hby_State* h) {
  release(h, Parser, h->parser);
  free_table(h, &h->globals);
  free_table(h, &h->global_consts);
  free_table(h, &h->strs);
  free_table(h, &h->files);
  free_objs(h);
  free(h);
}

void hby_cli_args(hby_State* h, int argc, const char** args) {
  // free is equivalent to clearing
  free_varr(h, &h->args->varr);
  for (int i = 0; i < argc; i++) {
    const char* arg = args[i];
    int len = (int)strlen(arg);
    Val val_arg = create_obj(copy_str(h, arg, len));
    push(h, val_arg);
    push_varr(h, &h->args->varr, val_arg);
    pop(h);
  }
}

void reset_stack(hby_State* h) {
  h->top = h->stack;
  h->frame = h->frame_stack;
  h->open_upvals = NULL;
}
