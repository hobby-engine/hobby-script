#ifndef __HBS_MEM_H
#define __HBS_MEM_H

#include "common.h"
#include "state.h"

#define allocate(h, T, num) (T*)reallocate(h, NULL, 0, sizeof(T) * (num))
#define release(h, T, ptr) reallocate(h, ptr, sizeof(T), 0)

#define grow_cap(cap) ((cap) < 8 ? 8 : (cap) * 2)
#define grow_arr(h, T, ptr, plen, len) \
  (T*)reallocate(h, ptr, sizeof(T) * (plen), sizeof(T) * len)
#define release_arr(h, T, ptr, plen) \
  reallocate(h, ptr, sizeof(T) * (plen), 0)

void* reallocate(hbs_State* h, void* ptr, size_t plen, size_t len);
void free_objs(hbs_State* h);

void gc(hbs_State* h);
void mark_obj(hbs_State* h, GcObj* obj);
void mark_val(hbs_State* h, Val val);

#endif // __HBS_MEM_H
