#include "map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "state.h"
#include "mem.h"
#include "obj.h"
#include "val.h"

#define map_max_load 0.75

void init_map(Map* map) {
  map->count = 0;
  map->cap = 0;
  map->items = NULL;
}

void free_map(hbs_State* h, Map* map) {
  free_arr(h, MapItem, map->items, map->cap);
  init_map(map);
}

static MapItem* find_item(MapItem* items, int cap, GcStr* k) {
  u32 idx = k->hash & (cap - 1);
  MapItem* tombstone = NULL;
  while (true) {
    MapItem* item = &items[idx];
    if (item->k == NULL) {
      if (is_null(item->v)) {
        // This entry is empty. Return result.
        return tombstone != NULL ? tombstone : item;
      } else {
        if (tombstone == NULL) {
          tombstone = item;
        }
      }
    } else if (item->k == k) {
      return item;
    }

    idx = (idx + 1) & (cap - 1);
  }
}

static void adjust_cap(hbs_State* h, Map* map, int cap) {
  MapItem* items = allocate(h, MapItem, cap);
  for (int i = 0; i < cap; i++) {
    items[i].k = NULL;
    items[i].v = create_null();
  }

  map->count = 0;
  for (int i = 0; i < map->cap; i++) {
    MapItem* item = &map->items[i];
    if (item->k == NULL) {
      continue;
    }

    MapItem* dst = find_item(items, cap, item->k);
    dst->k = item->k;
    dst->v = item->v;
    map->count++;
  }

  free_arr(h, MapItem, map->items, map->cap);
  map->items = items;
  map->cap = cap;
}

bool get_map(Map* map, GcStr* k, Val* out_v) {
  if (map->count == 0) {
    return false;
  }

  MapItem* item = find_item(map->items, map->cap, k);
  if (item->k == NULL) {
    return false;
  }

  *out_v = item->v;
  return true;
}

bool set_map(hbs_State* h, Map* map, GcStr* k, Val v) {
  if (map->count + 1 > map->cap * map_max_load) {
    int cap = grow_cap(map->cap);
    adjust_cap(h, map, cap);
  }

  MapItem* item = find_item(map->items, map->cap, k);
  bool is_new = item->k == NULL;
  if (is_new && is_null(item->v)) {
    map->count++;
  }

  item->k = k;
  item->v = v;
  return is_new;
}

bool rem_map(Map* map, GcStr* k) {
  if (map->count == 0) {
    return false;
  }

  MapItem* item = find_item(map->items, map->cap, k);
  if (item->k == NULL) {
    return false;
  }

  item->k = NULL;
  item->v = create_bool(true);
  return true;
}

void copy_map(struct hbs_State* h, Map* src, Map* dst) {
  for (int i = 0; i < src->cap; i++) {
    MapItem* item = &src->items[i];
    if (item->k != NULL) {
      set_map(h, dst, item->k, item->v);
    }
  }
}

GcStr* find_str_map(Map* map, const char* chars, int len, u32 hash) {
  if (map->count == 0) {
    return NULL;
  }

  u32 idx = hash & (map->cap - 1);

  while (true) {
    MapItem* item = &map->items[idx];
    if (item->k == NULL) {
      if (is_null(item->v)) {
        // We reached the end.
        return NULL;
      }
    } else if (item->k->len == len && item->k->hash == hash 
        && memcmp(item->k->chars, chars, len) == 0) {
      return item->k;
    }

    idx = (idx + 1) & (map->cap - 1);
  }
}

void mark_map(hbs_State* h, Map* map) {
  for (int i = 0; i < map->cap; i++) {
    MapItem* item = &map->items[i];
    mark_obj(h, (GcObj*)item->k);
    mark_val(h, item->v);
  }
}

void rem_black_map(struct hbs_State* h, Map* map) {
  for (int i = 0; i < map->cap; i++) {
    MapItem* item = &map->items[i];
    if (item->k != NULL && !item->k->obj.marked) {
      rem_map(map, item->k);
    }
  }
}
