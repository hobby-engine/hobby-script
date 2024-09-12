#include "hbs.h"
#include <math.h>

#define pi      3.14159265358979323846264338327950288419716939937510
#define tau     (pi * 2)
#define e       2.71828182845904523536028747135266249775724709369995
#define phi     1.6180339887498948482045868343656381177203091798057628621
#define emasch  0.57721566490153286060651209008240243104215933593992
#define deg2rad (180 / pi)
#define rad2deg (pi / 180)

bool math_abs(hbs_State* h, int argc) {
  hbs_push_num(h, fabs(hbs_get_num(h, 1)));
  return true;
}

bool math_acos(hbs_State* h, int argc) {
  hbs_push_num(h, acos(hbs_get_num(h, 1)));
  return true;
}

bool math_asin(hbs_State* h, int argc) {
  hbs_push_num(h, asin(hbs_get_num(h, 1)));
  return true;
}

bool math_atan(hbs_State* h, int argc) {
  hbs_push_num(h, atan(hbs_get_num(h, 1)));
  return true;
}

bool math_atan2(hbs_State* h, int argc) {
  hbs_push_num(h, atan2(hbs_get_num(h, 1), hbs_get_num(h, 2)));
  return true;
}

bool math_ceil(hbs_State* h, int argc) {
  hbs_push_num(h, ceil(hbs_get_num(h, 1)));
  return true;
}

bool math_clamp(hbs_State* h, int argc) {
  double mi = hbs_get_num(h, 1);
  double ma = hbs_get_num(h, 2);
  double n = hbs_get_num(h, 3);

  hbs_push_num(h, fmax(fmin(ma, n), mi));
  return true;
}

bool math_cos(hbs_State* h, int argc) {
  hbs_push_num(h, cos(hbs_get_num(h, 1)));
  return true;
}

bool math_cosh(hbs_State* h, int argc) {
  hbs_push_num(h, cosh(hbs_get_num(h, 1)));
  return true;
}

bool math_exp(hbs_State* h, int argc) {
  hbs_push_num(h, exp(hbs_get_num(h, 1)));
  return true;
}

bool math_floor(hbs_State* h, int argc) {
  hbs_push_num(h, floor(hbs_get_num(h, 1)));
  return true;
}

bool math_frac(hbs_State* h, int argc) {
  double i;
  hbs_push_num(h, modf(hbs_get_num(h, 1), &i));
  return true;
}

bool math_isnan(hbs_State* h, int argc) {
  hbs_push_bool(h, isnan(hbs_get_num(h, 1)));
  return true;
}

bool math_isinf(hbs_State* h, int argc) {
  hbs_push_bool(h, isinf(hbs_get_num(h, 1)));
  return true;
}

bool math_lerp(hbs_State* h, int argc) {
  double a = hbs_get_num(h, 1);
  double b = hbs_get_num(h, 2);
  double t = hbs_get_num(h, 3);
  hbs_push_num(h, (b - a) * t + a);
  return true;
}

bool math_log(hbs_State* h, int argc) {
  hbs_push_num(h, log(hbs_get_num(h, 1)));
  return true;
}

bool math_log10(hbs_State* h, int argc) {
  hbs_push_num(h, log10(hbs_get_num(h, 1)));
  return true;
}

bool math_max(hbs_State* h, int argc) {
  hbs_push_num(h, fmax(hbs_get_num(h, 1), hbs_get_num(h, 2)));
  return true;
}

bool math_min(hbs_State* h, int argc) {
  hbs_push_num(h, fmin(hbs_get_num(h, 1), hbs_get_num(h, 2)));
  return true;
}

bool math_pow(hbs_State* h, int argc) {
  hbs_push_num(h, pow(hbs_get_num(h, 1), hbs_get_num(h, 2)));
  return true;
}

bool math_round(hbs_State* h, int argc) {
  hbs_push_num(h, floor(hbs_get_num(h, 1) + 0.5));
  return true;
}

bool math_sin(hbs_State* h, int argc) {
  hbs_push_num(h, sin(hbs_get_num(h, 1)));
  return true;
}

bool math_sinh(hbs_State* h, int argc) {
  hbs_push_num(h, sinh(hbs_get_num(h, 1)));
  return true;
}

bool math_sqrt(hbs_State* h, int argc) {
  hbs_push_num(h, sqrt(hbs_get_num(h, 1)));
  return true;
}

bool math_tan(hbs_State* h, int argc) {
  hbs_push_num(h, tan(hbs_get_num(h, 1)));
  return true;
}

bool math_tanh(hbs_State* h, int argc) {
  hbs_push_num(h, tanh(hbs_get_num(h, 1)));
  return true;
}

hbs_StructMethod math_mod[] = {
  {"abs", math_abs, 1, hbs_static_fn},
  {"acos", math_acos, 1, hbs_static_fn},
  {"asin", math_asin, 1, hbs_static_fn},
  {"atan", math_atan, 1, hbs_static_fn},
  {"atan2", math_atan2, 2, hbs_static_fn},
  {"ceil", math_ceil, 1, hbs_static_fn},
  {"clamp", math_clamp, 3, hbs_static_fn},
  {"cos", math_cos, 1, hbs_static_fn},
  {"cosh", math_cosh, 1, hbs_static_fn},
  {"exp", math_exp, 1, hbs_static_fn},
  {"floor", math_floor, 1, hbs_static_fn},
  {"frac", math_frac, 1, hbs_static_fn},
  {"isnan", math_isnan, 1, hbs_static_fn},
  {"isinf", math_isinf, 1, hbs_static_fn},
  {"lerp", math_lerp, 3, hbs_static_fn},
  {"log", math_log, 1, hbs_static_fn},
  {"log10", math_log10, 1, hbs_static_fn},
  {"max", math_max, 2, hbs_static_fn},
  {"min", math_min, 2, hbs_static_fn},
  {"pow", math_pow, 2, hbs_static_fn},
  {"round", math_round, 1, hbs_static_fn},
  {"sin", math_sin, 1, hbs_static_fn},
  {"sinh", math_sinh, 1, hbs_static_fn},
  {"sqrt", math_sqrt, 1, hbs_static_fn},
  {"tan", math_tan, 1, hbs_static_fn},
  {"tanh", math_tanh, 1, hbs_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_math(hbs_State* h, int argc) {
  hbs_push_struct(h, "math");
  hbs_add_members(h, math_mod, -2);

  hbs_push_num(h, pi);
  hbs_add_static_const(h, "pi", -2);
  hbs_push_num(h, tau);
  hbs_add_static_const(h, "tau", -2);
  hbs_push_num(h, e);
  hbs_add_static_const(h, "e", -2);
  hbs_push_num(h, phi);
  hbs_add_static_const(h, "phi", -2);
  hbs_push_num(h, emasch);
  hbs_add_static_const(h, "emasch", -2);
  hbs_push_num(h, deg2rad);
  hbs_add_static_const(h, "deg2rad", -2);
  hbs_push_num(h, rad2deg);
  hbs_add_static_const(h, "rad2deg", -2);

  hbs_set_global(h, "math");
  hbs_pop(h, 2);
  
  return false;
}
