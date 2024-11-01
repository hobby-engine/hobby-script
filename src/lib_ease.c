#include <math.h>
#include "hby.h"
#include "lib.h"

#define pi 3.14159265358979323846264338327950288419716939937510

static bool ease_sine_in(hby_State* h, int argc) {
  double x = hby_get_num(h, 1);
  hby_push_num(h, 1 - cos((x * pi) / 2));
  return true;
}

static bool ease_sine_out(hby_State* h, int argc) {
  double x = hby_get_num(h, 1);
  hby_push_num(h, sin((x * pi) / 2));
  return true;
}

static bool ease_sine_inout(hby_State* h, int argc) {
  double x = hby_get_num(h, 1);
  hby_push_num(h, -(cos(pi * x) - 1) / 2);
  return true;
}

hby_StructMethod ease_mod[] = {
  {"sine_in", ease_sine_in, 1, hby_static_fn},
  {"sine_out", ease_sine_out, 1, hby_static_fn},
  {"sine_inout", ease_sine_inout, 1, hby_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_ease(hby_State* h, int argc) {
  hby_push_struct(h, "ease");
  hby_struct_add_members(h, ease_mod, -1);
  hby_set_global(h, NULL);
  hby_pop(h, 2);

  return false;
}
