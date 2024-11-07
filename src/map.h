#ifndef __HBY_MAP_H
#define __HBY_MAP_H

#include "common.h"
#include "obj.h"
#include "state.h"

bool get_map(GcMap* map, Val k, Val* out_v);
bool set_map(hby_State* h, GcMap* map, Val k, Val v);
bool rem_map(hby_State* h, GcMap* map, Val k);
void mark_map(hby_State* h, GcMap* map);

#endif // __HBY_MAP_H
