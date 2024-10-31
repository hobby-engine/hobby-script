#include "lib.h"

#include <string.h>
#include <time.h>
#include "hby.h"
#include "state.h"

static bool sys_clock(hby_State* h, int argc) {
  hby_push_num(h, (double)clock() / CLOCKS_PER_SEC);
  return true;
}

static bool sys_command(hby_State* h, int argc) {
  hby_push_num(h, system(hby_get_string(h, 1, NULL)));
  return true;
}

static bool sys_getenv(hby_State* h, int argc) {
  const char* chars = getenv(hby_get_string(h, 1, NULL));
  hby_push_string_copy(h, chars, strlen(chars));
  return true;
}

static bool sys_exit(hby_State* h, int argc) {
  exit(hby_get_num(h, 1));
  return false;
}

static bool sys_cliargs(hby_State* h, int argc) {
  // TODO: update this when arrays on the C API are supported
  push(h, create_obj(h->args));
  return true;
}

hby_StructMethod sys_mod[] = {
  {"clock", sys_clock, 0, hby_static_fn},
  {"command", sys_command, 1, hby_static_fn},
  {"getenv", sys_getenv, 1, hby_static_fn},
  {"exit", sys_exit, 1, hby_static_fn},
  {"cliargs", sys_cliargs, 0, hby_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_sys(hby_State* h, int argc) {
  hby_push_struct(h, "sys");
  hby_add_members(h, sys_mod, -2);
  hby_set_global(h, "sys");
  hby_pop(h, 2);

  return false;
}
