#include <math.h>
#include "hbs.h"
#include "lib.h"

#define pi 3.14159265358979323846264338327950288419716939937510

static bool ease_sine_in(hbs_State* h, int argc) {
  double x = hbs_get_num(h, 1);
  hbs_push_num(h, 1 - cos((x * pi) / 2));
  return true;
}

static bool ease_sine_out(hbs_State* h, int argc) {
  double x = hbs_get_num(h, 1);
  hbs_push_num(h, sin((x * pi) / 2));
  return true;
}

static bool ease_sine_inout(hbs_State* h, int argc) {
  double x = hbs_get_num(h, 1);
  hbs_push_num(h, -(cos(pi * x) - 1) / 2);
  return true;
}

hbs_StructMethod ease_mod[] = {
  {"sine_in", ease_sine_in, 1, hbs_static_fn},
  {"sine_out", ease_sine_out, 1, hbs_static_fn},
  {"sine_inout", ease_sine_inout, 1, hbs_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_ease(hbs_State* h, int argc) {
  hbs_push_struct(h, "ease");
  hbs_add_members(h, ease_mod, -2);
  hbs_set_global(h, NULL);
  hbs_pop(h, 2);

  return false;
}
