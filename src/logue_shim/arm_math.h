/* PrLogueXXL: portable stand-ins for the CMSIS names logue-sdk's cortexm4.h maps, so KORG's oscillator sources build
 * natively (armv7 Linux / x86). Only plain C; same results as the Cortex-M4 instructions. */
#pragma once
#include <stdint.h>
typedef int8_t q7_t; typedef int16_t q15_t; typedef int32_t q31_t; typedef int64_t q63_t; typedef float float32_t;
static inline int32_t __SSAT(int32_t v, uint32_t b) { int32_t hi = (1 << (b - 1)) - 1, lo = -(1 << (b - 1));
  return v > hi ? hi : v < lo ? lo : v; }
static inline uint32_t __USAT(int32_t v, uint32_t b) { int32_t hi = (int32_t)((1u << b) - 1); return v < 0 ? 0 : v > hi ? hi : v; }
static inline int32_t __QADD(int32_t a, int32_t b) { int64_t s = (int64_t)a + b; return s > INT32_MAX ? INT32_MAX : s < INT32_MIN ? INT32_MIN : (int32_t)s; }
static inline int32_t __QSUB(int32_t a, int32_t b) { int64_t s = (int64_t)a - b; return s > INT32_MAX ? INT32_MAX : s < INT32_MIN ? INT32_MIN : (int32_t)s; }
static inline uint32_t __CLZ(uint32_t v) { return v ? (uint32_t)__builtin_clz(v) : 32; }
static inline int32_t __SMMLA(int32_t a, int32_t b, int32_t c) { return c + (int32_t)(((int64_t)a * b) >> 32); }
static inline uint32_t __REV(uint32_t v) { return __builtin_bswap32(v); }
static inline uint32_t __RBIT(uint32_t v) { uint32_t r = 0; for (int i = 0; i < 32; i++) { r = (r << 1) | (v & 1); v >>= 1; } return r; }
#define __NOP() ((void)0)
#define __SIMD32_TYPE int32_t
/* packed 16-bit ops with the GE flags SEL reads (fixed_math.h's q15max/q15min use qsub16 then sel) */
static __thread uint32_t s_ge;
static inline int16_t s16sat(int32_t v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v; }
static inline int32_t __QSUB16(int32_t a, int32_t b) {
  int32_t lo = (int16_t)a - (int16_t)b, hi = (int16_t)(a >> 16) - (int16_t)(b >> 16);
  s_ge = (lo >= 0 ? 0x3u : 0) | (hi >= 0 ? 0xCu : 0);
  return (int32_t)(((uint32_t)(uint16_t)s16sat(hi) << 16) | (uint16_t)s16sat(lo)); }
static inline int32_t __QADD16(int32_t a, int32_t b) {
  int32_t lo = (int16_t)a + (int16_t)b, hi = (int16_t)(a >> 16) + (int16_t)(b >> 16);
  return (int32_t)(((uint32_t)(uint16_t)s16sat(hi) << 16) | (uint16_t)s16sat(lo)); }
static inline int32_t __SEL(int32_t a, int32_t b) {
  uint32_t m = ((s_ge & 1) ? 0xFFu : 0) | ((s_ge & 2) ? 0xFF00u : 0) | ((s_ge & 4) ? 0xFF0000u : 0) | ((s_ge & 8) ? 0xFF000000u : 0);
  return (int32_t)(((uint32_t)a & m) | ((uint32_t)b & ~m)); }
