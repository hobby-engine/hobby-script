#include <stdio.h>
#include "hby.h"
#include "mem.h"

static bool io_write(hby_State* h, int argc) {
  size_t len;
  const char* str = hby_get_string(h, 1, &len);
  fwrite(str, sizeof(char), len, stdout);
  return false;
}

static bool io_print(hby_State* h, int argc) {
  for (int i = 1; i <= argc; i++) {
    const char* s = hby_get_tostring(h, i, NULL);
    if (i > 1) {
      putc('\t', stdout);
    }
    fputs(s, stdout);
  }
  putc('\n', stdout);

  return false;
}

static bool io_input(hby_State* h, int argc) {
  int cap = 8;
  int len = 0;
  char* i = allocate(h, char, cap);

  int c = EOF;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (len + 1 > cap) {
      int old_cap = cap;
      cap = grow_cap(cap);
      i = grow_arr(h, char, i, old_cap, cap);
    }
    i[len++] = c;
  }

  i = grow_arr(h, char, i, cap, len + 1);
  i[len] = '\0';

  hby_push_string(h, i, len);
  return true;
}

hby_StructMethod io[] = {
  {"print", io_print, -1, hby_static_fn},
  {"write", io_write,  1, hby_static_fn},
  {"input", io_input,  0, hby_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_io(hby_State* h, int argc) {
  hby_push_struct(h, "io");
  hby_add_members(h, io, -2);
  hby_set_global(h, "io");
  hby_pop(h, 2);

  return false;
}
