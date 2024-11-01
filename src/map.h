#ifndef __HBY_MAP_H
#define __HBY_MAP_H

#include "common.h"
#include "val.h"

struct hby_State;

typedef struct {
  GcStr* k;
  Val v;
} MapItem;

typedef struct {
  int count;
  int cap;
  MapItem* items;
} Map;

void init_map(Map* map);
void free_map(struct hby_State* h, Map* map);
bool get_map(Map* map, GcStr* k, Val* out_v);
bool set_map(struct hby_State* h, Map* map, GcStr* k, Val v);
bool rem_map(Map* map, GcStr* k);
void copy_map(struct hby_State* h, Map* src, Map* dst);
GcStr* find_str_map(Map* map, const char* chars, int len, u32 hash);
void mark_map(struct hby_State* h, Map* map);
void rem_black_map(struct hby_State* h, Map* map);

#endif // __HBY_MAP_H
