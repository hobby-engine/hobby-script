#ifndef __HBS_DEBUG_H
#define __HBS_DEBUG_H

#include "chunk.h"

void print_chunk(hbs_State* h, Chunk* c, const char* name);
int print_bc(hbs_State* h, Chunk *c, int idx);

#endif // __HBS_DEBUG_H
