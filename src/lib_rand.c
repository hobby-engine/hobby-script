#include "lib.h"

#include <math.h>
#include <time.h>
#include "common.h"
#include "hbs.h"

typedef struct {
  u64 a;
} Rng;

static u64 xorshift_next(Rng* rng) {
  rng->a ^= rng->a >> 12;
  rng->a ^= rng->a << 25;
  rng->a ^= rng->a >> 27;
  return rng->a * 2685821657736338717ULL;
}

static double next_double(Rng* rng) {
  u64 r = xorshift_next(rng);

  union {u64 i; double d;} u;
  u.i = ((0x3FFULL) << 52) | (r >> 12);

  return u.d - 1;
}

static double range(Rng* rng, double low, double high) {
  double r = next_double(rng);
  return r * (high - low) + low;
}

Rng rng = {1};

static bool rand_seed(hbs_State* h, int argc) {
  // TODO: Seed this better
  // We multiply by that big number because xorshift is bad when the seed is
  // close together
  rng.a = time(NULL) * 100000;
  return false;
}

static bool rand_irange(hbs_State* h, int argc) {
  int low = hbs_get_num(h, 1);
  int high = hbs_get_num(h, 2) + 1;
  hbs_push_num(h, floor(range(&rng, low, high)));
  return true;
}

static bool rand_frange(hbs_State* h, int argc) {
  int low = hbs_get_num(h, 1);
  int high = hbs_get_num(h, 2);
  hbs_push_num(h, range(&rng, low, high));
  return true;
}

static bool rand_next(hbs_State* h, int argc) {
  hbs_push_num(h, next_double(&rng));
  return true;
}

// TODO: Make it so you can instance random number generators instead of using
// a single global one
hbs_StructMethod rand_mod[] = {
  {"seed", rand_seed, 0, hbs_static_fn},
  {"next", rand_next, 0, hbs_static_fn},
  {"irange", rand_irange, 2, hbs_static_fn},
  {"frange", rand_frange, 2, hbs_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_rand(hbs_State* h, int argc) {
  hbs_push_struct(h, "rand");
  hbs_add_members(h, rand_mod, -2);
  hbs_set_global(h, "rand");
  hbs_pop(h, 2);

  return false;
}
