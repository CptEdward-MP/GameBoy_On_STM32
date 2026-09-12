#pragma once

#include <cstdint>
#include <stddef.h>

#if defined(_MSC_VER)
  #define f_inline __forceinline
#elif defined(__GNUC__) || defined(__clang__)
  #define f_inline inline __attribute__((always_inline))
#else
  #define f_inline inline
#endif

#if defined(_MSC_VER)
  #define rstk __restrict
#elif defined(__GNUC__) || defined(__clang__)
  #define rstk __restrict__
#else
  #define rstk
#endif



typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
