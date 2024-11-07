#include "lib.h"

#include "errmsg.h"
#include "state.h"
#include "tostr.h"
#include "hby.h"
#include "obj.h"
#include "map.h"
#include "mem.h"

static bool map_rem(hby_State* h, int argc) {
  GcMap* map = as_map(h->frame->base[0]);

  if (!rem_map(h, map, h->frame->base[1])) {
    hby_err(h, err_msg_undef_map_key, to_str(h, h->frame->base[1])->chars);
  }
  return false;
}

static bool map_has(hby_State* h, int argc) {
  GcMap* map = as_map(h->frame->base[0]);

  Val val;
  hby_push_bool(h, get_map(map, h->frame->base[1], &val));
  return true;
}

static bool map_keys(hby_State* h, int argc) {
  GcMap* map = as_map(h->frame->base[0]);

  hby_push_array(h);

  for (int i = 0; i < map->item_cap; i++) {
    MapItem* item = &map->items[i];
    if (is_null(item->key)) {
      continue;
    }

    push(h, item->key);
    hby_array_add(h, -2);
  }

  return true;
}

static bool map_clear(hby_State* h, int argc) {
  GcMap* map = as_map(h->frame->base[0]);
  release_arr(h, MapItem, map->items, map->item_cap);
  map->items = NULL;
  map->itemc = 0;
  map->item_cap = 0;
  return false;
}

hby_StructMethod map_methods[] = {
  {"rem", map_rem, 1, hby_method},
  {"has", map_has, 1, hby_method},
  {"keys", map_keys, 0, hby_method},
  {"clear", map_clear, 0, hby_method},
  {NULL, NULL, 0, 0},
};

bool open_map(hby_State* h, int argc) {
  hby_push_struct(h, "Map");
  h->map_struct = as_struct(*(h->top - 1));
  hby_struct_add_members(h, map_methods, -1);
  hby_set_global(h, NULL, -1);

  return false;
}
