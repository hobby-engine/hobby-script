#ifndef __HBS_LIB_H
#define __HBS_LIB_H

#include <stdbool.h>
#include "hbs.h"

bool open_core(hbs_State* h, int argc);
bool open_io(hbs_State* h, int argc);
bool open_math(hbs_State* h, int argc);
bool open_rand(hbs_State* h, int argc);
bool open_sys(hbs_State* h, int argc);
bool open_arr(hbs_State* h, int argc);

#endif // __HBS_LIB_H
