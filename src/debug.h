#ifndef __HBY_DEBUG_H
#define __HBY_DEBUG_H

#include "chunk.h"

void print_chunk(hby_State* h, Chunk* c, const char* name);
int print_bc(hby_State* h, Chunk *c, int index);

#endif // __HBY_DEBUG_H
