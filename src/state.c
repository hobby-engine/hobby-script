#include "state.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "tostr.h"
#include "compiler.h"
#include "mem.h"
#include "obj.h"
#include "hby.h"
#include "lib.h"

hby_State* create_state() {
  hby_State* h = (hby_State*)malloc(sizeof(hby_State));

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

  h->args = create_arr(h);

  init_map(&h->globals);
  init_map(&h->strs);
  init_map(&h->files);
  reset_stack(h);

  srand(time(NULL));

  // Using the C API like this is dangerous, since it cannot resolve errors.
  // TODO: Add a way to call API functions outside of the runtime
  // This all should be deferred until the VM starts.
  hby_open_lib(h, open_arr);
  hby_open_lib(h, open_core);
  hby_open_lib(h, open_ease);
  hby_open_lib(h, open_io);
  hby_open_lib(h, open_math);
  hby_open_lib(h, open_rand);
  hby_open_lib(h, open_str);
  hby_open_lib(h, open_sys);

  return h;
}

void free_state(hby_State* h) {
  release(h, Parser, h->parser);
  free_map(h, &h->globals);
  free_map(h, &h->strs);
  free_map(h, &h->files);
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

bool file_imported(hby_State* h, const char* name) {
  Val val;
  return get_map(&h->files, copy_str(h, name, strlen(name)), &val);
}

void reset_stack(hby_State* h) {
  h->top = h->stack;
  h->frame = h->frame_stack;
  h->open_upvals = NULL;
}
