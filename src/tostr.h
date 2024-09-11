#ifndef __HBS_TOSTRING_H
#define __HBS_TOSTRING_H

#include "common.h"
#include "state.h"
#include "val.h"

GcStr* to_str(hbs_State* h, Val val);
GcStr* num_to_str(hbs_State* h, double n);
GcStr* bool_to_str(hbs_State* h, bool b);
GcStr* str_fmt(hbs_State* h, const char* fmt, ...);

#endif // __HBS_TOSTRING_H
