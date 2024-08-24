#pragma once

#include <cstdint>
#include <unistd.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#ifdef __AVX512F__
#define SIMD_BYTES 64
#else
#define SIMD_BYTES 32
#endif
constexpr u64 SIMD_FLOATS = (SIMD_BYTES/sizeof(f32));

#define ASSERT(expr)        \
{                           \
    if (!(expr))            \
    {                       \
        fprintf(stderr, "Assertion: %s failed in %s:%d\n", #expr, __FILE__, __LINE__);\
        _exit(1);           \
    }                       \
}
