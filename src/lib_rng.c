#include "lib.h"

#include <math.h>
#include <time.h>
#include "common.h"
#include "hby.h"

typedef struct {
  u64 a;
} Rng;

static u64 xorshift_next(Rng* rng) {
  rng->a ^= rng->a >> 12;
  rng->a ^= rng->a << 25;
  rng->a ^= rng->a >> 27;
  return rng->a * 2685821657736338717ULL;
}

// Thomas Wang's 64-bit integer hashing function:
// https://web.archive.org/web/20110807030012/http://www.cris.com/%7ETtwang/tech/inthash.htm
static u64 wang_hash64(u64 key)
{
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
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

static bool rng_new(hby_State* h, int argc) {
  Rng* rng = (Rng*)hby_push_udata(h, sizeof(Rng));
  hby_get_global(h, "Rng");
  hby_udata_set_metastruct(h, -2);
  rng->a = wang_hash64(time(NULL));
  return true;
}

static bool rng_seed(hby_State* h, int argc) {
  int seed = time(NULL);
  if (argc == 1) {
    seed = hby_get_num(h, 1);
  } else if (argc > 1) {
    hby_err(h, "Expected 0-1 arguments.");
  }

  Rng* rng = (Rng*)hby_get_udata(h, 0);
  rng->a = wang_hash64(seed);
  return false;
}

static bool rng_next(hby_State* h, int argc) {
  Rng* rng = (Rng*)hby_get_udata(h, 0);
  hby_push_num(h, next_double(rng));
  return true;
}

static bool rng_bool(hby_State* h, int argc) {
  Rng* rng = (Rng*)hby_get_udata(h, 0);
  hby_push_bool(h, next_double(rng) < 0.5);
  return true;
}

static bool rng_pick(hby_State* h, int argc) {
  Rng* rng = (Rng*)hby_get_udata(h, 0);
  hby_expect_array(h, 1);
  int len = hby_len(h, 1);
  int index = floor(range(rng, 0, len));
  hby_array_index(h, 1, index);
  return true;
}

static bool rng_irange(hby_State* h, int argc) {
  Rng* rng = (Rng*)hby_get_udata(h, 0);
  int low = hby_get_num(h, 1);
  int high = hby_get_num(h, 2) + 1;
  hby_push_num(h, floor(range(rng, low, high)));
  return true;
}

static bool rng_frange(hby_State* h, int argc) {
  Rng* rng = (Rng*)hby_get_udata(h, 0);
  int low = hby_get_num(h, 1);
  int high = hby_get_num(h, 2);
  hby_push_num(h, range(rng, low, high));
  return true;
}

hby_StructMethod rng_struct[] = {
  {"new", rng_new, 0, hby_static_fn},
  {"seed", rng_seed, -1, hby_method},
  {"next", rng_next, 0, hby_method},
  {"bool", rng_bool, 0, hby_method},
  {"pick", rng_pick, 1, hby_method},
  {"irange", rng_irange, 2, hby_method},
  {"frange", rng_frange, 2, hby_method},
  {NULL, NULL, 0, 0},
};

bool open_rng(hby_State* h, int argc) {
  hby_push_struct(h, "Rng");
  hby_struct_add_members(h, rng_struct, -1);
  hby_set_global(h, "Rng");

  return false;
}
