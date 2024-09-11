#ifndef __HBS_COMMON_H
#define __HBS_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define hbs_trace_exec
// #define hbs_print_bc

// #define hbs_stress_gc
// #define hbs_log_gc

#define nan_boxing

#define uint8_count (UINT8_MAX + 1)

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

#endif // __HBS_COMMON_H
