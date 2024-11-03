#include "hby.h"
#include <math.h>

#define pi      3.14159265358979323846264338327950288419716939937510
#define tau     (pi * 2)
#define e       2.71828182845904523536028747135266249775724709369995
#define phi     1.6180339887498948482045868343656381177203091798057628621
#define emasch  0.57721566490153286060651209008240243104215933593992
#define deg2rad (180 / pi)
#define rad2deg (pi / 180)

bool math_abs(hby_State* h, int argc) {
  hby_push_num(h, fabs(hby_get_num(h, 1)));
  return true;
}

bool math_acos(hby_State* h, int argc) {
  hby_push_num(h, acos(hby_get_num(h, 1)));
  return true;
}

bool math_asin(hby_State* h, int argc) {
  hby_push_num(h, asin(hby_get_num(h, 1)));
  return true;
}

bool math_atan(hby_State* h, int argc) {
  hby_push_num(h, atan(hby_get_num(h, 1)));
  return true;
}

bool math_atan2(hby_State* h, int argc) {
  hby_push_num(h, atan2(hby_get_num(h, 1), hby_get_num(h, 2)));
  return true;
}

bool math_ceil(hby_State* h, int argc) {
  hby_push_num(h, ceil(hby_get_num(h, 1)));
  return true;
}

bool math_clamp(hby_State* h, int argc) {
  double mi = hby_get_num(h, 1);
  double ma = hby_get_num(h, 2);
  double n = hby_get_num(h, 3);

  hby_push_num(h, fmax(fmin(ma, n), mi));
  return true;
}

bool math_cos(hby_State* h, int argc) {
  hby_push_num(h, cos(hby_get_num(h, 1)));
  return true;
}

bool math_cosh(hby_State* h, int argc) {
  hby_push_num(h, cosh(hby_get_num(h, 1)));
  return true;
}

bool math_exp(hby_State* h, int argc) {
  hby_push_num(h, exp(hby_get_num(h, 1)));
  return true;
}

bool math_floor(hby_State* h, int argc) {
  hby_push_num(h, floor(hby_get_num(h, 1)));
  return true;
}

bool math_frac(hby_State* h, int argc) {
  double i;
  hby_push_num(h, modf(hby_get_num(h, 1), &i));
  return true;
}

bool math_isnan(hby_State* h, int argc) {
  hby_push_bool(h, isnan(hby_get_num(h, 1)));
  return true;
}

bool math_isinf(hby_State* h, int argc) {
  hby_push_bool(h, isinf(hby_get_num(h, 1)));
  return true;
}

bool math_lerp(hby_State* h, int argc) {
  double a = hby_get_num(h, 1);
  double b = hby_get_num(h, 2);
  double t = hby_get_num(h, 3);
  hby_push_num(h, (b - a) * t + a);
  return true;
}

bool math_log(hby_State* h, int argc) {
  hby_push_num(h, log(hby_get_num(h, 1)));
  return true;
}

bool math_log10(hby_State* h, int argc) {
  hby_push_num(h, log10(hby_get_num(h, 1)));
  return true;
}

bool math_max(hby_State* h, int argc) {
  hby_push_num(h, fmax(hby_get_num(h, 1), hby_get_num(h, 2)));
  return true;
}

bool math_min(hby_State* h, int argc) {
  hby_push_num(h, fmin(hby_get_num(h, 1), hby_get_num(h, 2)));
  return true;
}

bool math_pow(hby_State* h, int argc) {
  hby_push_num(h, pow(hby_get_num(h, 1), hby_get_num(h, 2)));
  return true;
}

bool math_round(hby_State* h, int argc) {
  hby_push_num(h, floor(hby_get_num(h, 1) + 0.5));
  return true;
}

bool math_sin(hby_State* h, int argc) {
  hby_push_num(h, sin(hby_get_num(h, 1)));
  return true;
}

bool math_sinh(hby_State* h, int argc) {
  hby_push_num(h, sinh(hby_get_num(h, 1)));
  return true;
}

bool math_sqrt(hby_State* h, int argc) {
  hby_push_num(h, sqrt(hby_get_num(h, 1)));
  return true;
}

bool math_tan(hby_State* h, int argc) {
  hby_push_num(h, tan(hby_get_num(h, 1)));
  return true;
}

bool math_tanh(hby_State* h, int argc) {
  hby_push_num(h, tanh(hby_get_num(h, 1)));
  return true;
}

hby_StructMethod math_mod[] = {
  {"abs", math_abs, 1, hby_static_fn},
  {"acos", math_acos, 1, hby_static_fn},
  {"asin", math_asin, 1, hby_static_fn},
  {"atan", math_atan, 1, hby_static_fn},
  {"atan2", math_atan2, 2, hby_static_fn},
  {"ceil", math_ceil, 1, hby_static_fn},
  {"clamp", math_clamp, 3, hby_static_fn},
  {"cos", math_cos, 1, hby_static_fn},
  {"cosh", math_cosh, 1, hby_static_fn},
  {"exp", math_exp, 1, hby_static_fn},
  {"floor", math_floor, 1, hby_static_fn},
  {"frac", math_frac, 1, hby_static_fn},
  {"isnan", math_isnan, 1, hby_static_fn},
  {"isinf", math_isinf, 1, hby_static_fn},
  {"lerp", math_lerp, 3, hby_static_fn},
  {"log", math_log, 1, hby_static_fn},
  {"log10", math_log10, 1, hby_static_fn},
  {"max", math_max, 2, hby_static_fn},
  {"min", math_min, 2, hby_static_fn},
  {"pow", math_pow, 2, hby_static_fn},
  {"round", math_round, 1, hby_static_fn},
  {"sin", math_sin, 1, hby_static_fn},
  {"sinh", math_sinh, 1, hby_static_fn},
  {"sqrt", math_sqrt, 1, hby_static_fn},
  {"tan", math_tan, 1, hby_static_fn},
  {"tanh", math_tanh, 1, hby_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_math(hby_State* h, int argc) {
  hby_push_struct(h, "math");
  hby_struct_add_members(h, math_mod, -1);

  hby_push_num(h, pi);
  hby_struct_add_const(h, "pi", -2);
  hby_push_num(h, tau);
  hby_struct_add_const(h, "tau", -2);
  hby_push_num(h, e);
  hby_struct_add_const(h, "e", -2);
  hby_push_num(h, phi);
  hby_struct_add_const(h, "phi", -2);
  hby_push_num(h, emasch);
  hby_struct_add_const(h, "emasch", -2);
  hby_push_num(h, deg2rad);
  hby_struct_add_const(h, "deg2rad", -2);
  hby_push_num(h, rad2deg);
  hby_struct_add_const(h, "rad2deg", -2);
  hby_push_num(h, NAN);
  hby_struct_add_const(h, "nan", -2);
  hby_push_num(h, INFINITY);
  hby_struct_add_const(h, "inf", -2);

  hby_set_global(h, "math", -1);
  hby_pop(h, 2);
  
  return false;
}
