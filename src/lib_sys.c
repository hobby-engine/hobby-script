#include "lib.h"

#include <string.h>
#include <time.h>
#include "hbs.h"

static bool sys_clock(hbs_State* h, int argc) {
  hbs_push_num(h, (double)clock() / CLOCKS_PER_SEC);
  return true;
}

static bool sys_command(hbs_State* h, int argc) {
  hbs_push_num(h, system(hbs_get_str(h, 1, NULL)));
  return true;
}

static bool sys_getenv(hbs_State* h, int argc) {
  const char* chars = getenv(hbs_get_str(h, 1, NULL));
  hbs_push_str_copy(h, chars, strlen(chars));
  return true;
}

static bool sys_exit(hbs_State* h, int argc) {
  exit(hbs_get_num(h, 1));
  return false;
}

hbs_StructMethod sys_mod[] = {
  {"clock", sys_clock, 0, hbs_static_fn},
  {"command", sys_command, 1, hbs_static_fn},
  {"getenv", sys_getenv, 1, hbs_static_fn},
  {"exit", sys_exit, 1, hbs_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_sys(hbs_State* h, int argc) {
  hbs_define_struct(h, "sys");
  hbs_add_members(h, sys_mod, -2);
  hbs_set_global(h, "sys");
  hbs_pop(h, 2);

  return false;
}
