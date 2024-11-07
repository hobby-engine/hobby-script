#include "map.h"

#include "mem.h"

#define map_max_load 0.75

static u32 hash_val(Val* val) {
  if (is_str(*val)) {
    return as_str(*val)->hash;
  }
  return 0;
}

static MapItem* find_item(MapItem* items, int cap, Val k) {
  u32 idx = hash_val(&k) & (cap - 1);

  MapItem* tombstone = NULL;

  while (true) {
    MapItem* item = &items[idx];
    if (is_null(item->key)) {
      if (is_null(item->val)) {
        return tombstone != NULL ? tombstone : item;
      } else {
        if (tombstone == NULL) {
          tombstone = item;
        }
      }
    } else if (vals_eql(item->key, k)){
      return item;
    }

    idx = (idx + 1) & (cap - 1);
  }
}

static void adjust_cap(hby_State* h, GcMap* map, int cap) {
  MapItem* items = allocate(h, MapItem, cap);
  for (int i = 0; i < cap; i++) {
    items[i].key = create_null();
    items[i].val = create_null();
  }

  map->itemc = 0;
  for (int i = 0; i < map->item_cap; i++) {
    MapItem* item = &map->items[i];
    if (is_null(item->key)) {
      continue;
    }

    MapItem* dst = find_item(items, cap, item->key);
    dst->key = item->key;
    dst->val = item->val;
    map->itemc++;
  }

  release_arr(h, MapItem, map->items, map->item_cap);
  map->items = items;
  map->item_cap = cap;
}

bool get_map(GcMap* map, Val k, Val* out_v) {
  if (map->itemc == 0) {
    return false;
  }

  MapItem* item = find_item(map->items, map->item_cap, k);
  if (is_null(item->key)) {
    return false;
  }

  *out_v = item->val;
  return true;
}

bool set_map(hby_State* h, GcMap* map, Val k, Val v) {
  if (map->itemc + 1 > map->item_cap * map_max_load) {
    int cap = grow_cap(map->item_cap);
    adjust_cap(h, map, cap);
  }

  MapItem* item = find_item(map->items, map->item_cap, k);
  bool is_new = is_null(item->key);
  if (is_new && is_null(item->val)) {
    map->itemc++;
  }

  item->key = k;
  item->val = v;
  return is_new;
}

bool rem_map(hby_State* h, GcMap* map, Val k) {
  if (map->itemc == 0) {
    return false;
  }

  MapItem* item = find_item(map->items, map->item_cap, k);
  if (is_null(item->key)) {
    return false;
  }

  item->key = create_null();
  item->val = create_bool(true);
  return true;
}

void mark_map(hby_State* h, GcMap* map) {
  for (int i = 0; i < map->item_cap; i++) {
    MapItem* item = &map->items[i];
    mark_obj(h, (GcObj*)item->key);
    mark_val(h, item->val);
  }
}
