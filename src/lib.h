#ifndef __HBY_LIB_H
#define __HBY_LIB_H

#include <stdbool.h>
#include "hby.h"

bool open_arr(hby_State* h, int argc);
bool open_core(hby_State* h, int argc);
bool open_ease(hby_State* h, int argc);
bool open_io(hby_State* h, int argc);
bool open_math(hby_State* h, int argc);
bool open_rng(hby_State* h, int argc);
bool open_str(hby_State* h, int argc);
bool open_sys(hby_State* h, int argc);

#endif // __HBY_LIB_H
