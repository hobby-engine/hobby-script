#ifndef __HBY_TOSTRING_H
#define __HBY_TOSTRING_H

#include "common.h"
#include "state.h"
#include "val.h"

GcStr* to_str(hby_State* h, Val val);
GcStr* num_to_str(hby_State* h, double n);
GcStr* bool_to_str(hby_State* h, bool b);
GcStr* str_fmt(hby_State* h, const char* fmt, ...);

#endif // __HBY_TOSTRING_H
