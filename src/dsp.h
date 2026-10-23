/* PrLogueXXL -- small DSP building blocks (44.1 kHz, float; from DrLogueXXL). */
#pragma once
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define SR 44100.f

static inline float clampf(float x, float a, float b) { return x < a ? a : x > b ? b : x; }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
/* soft clip, cheap: odd polynomial into tanh-like curve */
static inline float sat(float x)
{
    if (x > 3.f) return 1.f;
    if (x < -3.f) return -1.f;
    return x * (27.f + x * x) / (27.f + 9.f * x * x);
}
/* sin(2 pi x) for any x, without libm: wrap to [-0.5, 0.5), fold to [-0.25, 0.25], odd Taylor to y^9 (error < 4e-6) */
static inline float fsin_ph(float x)
{
    float y, y2;
    x -= (float)(int)(x + (x >= 0.f ? 0.5f : -0.5f));
    if (x > 0.25f) x = 0.5f - x;
    else if (x < -0.25f) x = -0.5f - x;
    y = x * 6.28318531f;
    y2 = y * y;
    return y * (1.f + y2 * (-0.166666667f + y2 * (0.00833333333f + y2 * (-0.000198412698f + y2 * 2.75573192e-6f))));
}
static inline float fsin(float rad) { return fsin_ph(rad * 0.159154943f); }

static inline float midi_hz(float n) { return 440.f * exp2f((n - 69.f) / 12.f); }
/* exponential decay coefficient for a time constant in seconds (time to -60 dB) */
static inline float decay_coef(float sec) { return sec <= 0.0001f ? 0.f : expf(-6.9077553f / (sec * SR)); }
static inline float flush(float x) { return fabsf(x) < 1e-20f ? 0.f : x; }

/* white noise, xorshift32 */
typedef struct { uint32_t s; } rng_t;
static inline uint32_t rng_u32(rng_t *r) { uint32_t x = r->s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return r->s = x ? x : 0x9E3779B9u; }
static inline float rng_f(rng_t *r) { return (float)(int32_t)rng_u32(r) * (1.f / 2147483648.f); }   /* -1..1 */
static inline float rng_uf(rng_t *r) { return (float)(rng_u32(r) >> 8) * (1.f / 16777216.f); }       /* 0..1 */

/* state-variable filter (Chamberlin / TPT, Andrew Simper's form): low, band, high from one state */
typedef struct { float ic1, ic2; } svf_t;
typedef struct { float g, k, a1, a2, a3; } svfc_t;
static inline void svf_coef(svfc_t *c, float hz, float q)   /* q: 0.5 .. 40 */
{
    hz = clampf(hz, 10.f, SR * 0.49f);
    c->g = tanf((float)M_PI * hz / SR);
    c->k = 1.f / q;
    c->a1 = 1.f / (1.f + c->g * (c->g + c->k));
    c->a2 = c->g * c->a1;
    c->a3 = c->g * c->a2;
}
static inline void svf_tick(svf_t *s, const svfc_t *c, float x, float *lp, float *bp, float *hp)
{
    const float v3 = x - s->ic2;
    const float v1 = c->a1 * s->ic1 + c->a2 * v3;
    const float v2 = s->ic2 + c->a2 * s->ic1 + c->a3 * v3;
    s->ic1 = flush(2.f * v1 - s->ic1);
    s->ic2 = flush(2.f * v2 - s->ic2);
    *lp = v2; *bp = v1; *hp = x - c->k * v1 - v2;
}

/* one-pole low-pass */
typedef struct { float z, a; } lp1_t;
static inline void lp1_set(lp1_t *f, float hz) { f->a = 1.f - expf(-2.f * (float)M_PI * clampf(hz, 1.f, 22000.f) / SR); }
static inline float lp1_tick(lp1_t *f, float x) { f->z += f->a * (x - f->z); f->z = flush(f->z); return f->z; }

/* DC blocker */
typedef struct { float x1, y1; } dcb_t;
static inline float dcb_tick(dcb_t *d, float x) { const float y = x - d->x1 + 0.9975f * d->y1; d->x1 = x; d->y1 = flush(y); return y; }

/* constant-power pan, -1..1 -> gains (1 and 1 at centre, so a centred part keeps its level) */
static inline void pan_gains(float p, float *l, float *r)
{
    const float a = (p + 1.f) * 0.25f * (float)M_PI;
    *l = cosf(a) * 1.41421356f;
    *r = sinf(a) * 1.41421356f;
}
