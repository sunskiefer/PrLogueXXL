/* PrLogueXXL -- the logue SDK oscillator runtime (platform/prologue/inc/osc_api.h), re-implemented.
 *
 * The SDK declares these tables and functions but their contents live in the prologue's firmware (osc_api.syms:
 * wt_saw_lut_f = 0x08011db8 ...), so the data is not in the SDK. Here they are computed to the SDK's own definitions:
 *   - midi_to_hz_lut_f: 152 notes, equal temperament, A4 (note 69) = 440 Hz; k_note_max_hz caps the result;
 *   - wt_sine_lut_f: sin(2 pi x) over a half period, 128 + 1 points, mirrored and negated for x >= 0.5;
 *   - wt_saw/sqr/par_lut_f: 7 band-limited half-wave tables of 128 + 1 points each (same layout and the same lookup:
 *     linear interpolation, the second half read reversed and negated), table i holding only harmonics that stay
 *     below Nyquist at 48 kHz up to the highest note it serves (wt_*_notes = the first note of each table);
 *   - osc_white(): white noise in [-1, 1]; osc_rand(): 32-bit random.
 * The lookup functions are the SDK's code, unchanged apart from names (prefixed oa_ to stay out of the way of a unit's
 * own symbols). The wavetables A-F (wavesA[] ... wavesF[]) are KORG's firmware data and are not reproduced. */
#pragma once
#include <stdint.h>

#define OA_SR 48000
#define OA_SR_RECIP (2.08333333333333e-005f)
#define OA_MIDI_HZ_SIZE 152
#define OA_NOTE_MOD_FSCALE (0.00392156862745098f)
#define OA_NOTE_MAX_HZ (23679.643054f)
#define OA_WT_SIZE 128
#define OA_WT_MASK (OA_WT_SIZE - 1)
#define OA_WT_LUT (OA_WT_SIZE + 1)
#define OA_BL_CNT 7

extern float oa_midi_to_hz[OA_MIDI_HZ_SIZE];
extern float oa_sine[OA_WT_LUT];
extern float oa_saw[OA_BL_CNT * OA_WT_LUT], oa_sqr[OA_BL_CNT * OA_WT_LUT], oa_par[OA_BL_CNT * OA_WT_LUT];
extern uint8_t oa_saw_notes[OA_BL_CNT], oa_sqr_notes[OA_BL_CNT], oa_par_notes[OA_BL_CNT];

void oa_init(void);   /* builds the tables once per process (thread safe) */

static inline float oa_linint(float fr, float a, float b) { return a + fr * (b - a); }
static inline float oa_clipmax(float x, float m) { return x > m ? m : x; }

static inline float oa_notehz(uint8_t note) { return oa_midi_to_hz[note < OA_MIDI_HZ_SIZE ? note : OA_MIDI_HZ_SIZE - 1]; }
/* phase increment for note + mod/256 of a semitone (osc_w0f_for_note) */
static inline float oa_w0f_for_note(uint8_t note, uint8_t mod)
{
    const float f0 = oa_notehz(note), f1 = oa_notehz((uint8_t)(note + 1));
    return oa_clipmax(oa_linint(mod * OA_NOTE_MOD_FSCALE, f0, f1), OA_NOTE_MAX_HZ) * OA_SR_RECIP;
}

/* sin(2 pi x) (osc_sinf) */
static inline float oa_sinf(float x)
{
    const float p = x - (float)(uint32_t)x;
    const float x0f = 2.f * p * OA_WT_SIZE;
    const uint32_t x0p = (uint32_t)x0f;
    const uint32_t x0 = x0p & OA_WT_MASK, x1 = (x0 + 1) & OA_WT_MASK;
    const float y0 = oa_linint(x0f - (float)x0p, oa_sine[x0], oa_sine[x1]);
    return x0p < OA_WT_SIZE ? y0 : -y0;
}

/* the band-limited half-wave lookups (osc_bl_sawf / sqrf / parf and the interpolated *_bl2_*) */
static inline float oa_bl_half(const float *tab, float x, uint8_t idx)
{
    const float p = x - (float)(uint32_t)x;
    const float x0f = 2.f * p * OA_WT_SIZE;
    const uint32_t x0p = (uint32_t)x0f;
    uint32_t x0 = x0p, x1 = x0p + 1;
    float sign = 1.f;
    const float *wt = &tab[(idx < OA_BL_CNT ? idx : OA_BL_CNT - 1) * OA_WT_LUT];
    if (x0p >= OA_WT_SIZE) { x0 = OA_WT_SIZE - (x0p & OA_WT_MASK); x1 = x0 - 1; sign = -1.f; }
    return sign * oa_linint(x0f - (float)x0p, wt[x0], wt[x1]);
}
static inline float oa_bl2_half(const float *tab, float x, float idx)
{
    const uint8_t i0 = (uint8_t)idx;
    const float y0 = oa_bl_half(tab, x, i0);
    const float y1 = oa_bl_half(tab, x, (uint8_t)(i0 + 1 < OA_BL_CNT ? i0 + 1 : OA_BL_CNT - 1));
    return oa_linint(idx - (float)i0, y0, y1);
}
/* the parabola is symmetric: the second half is read reversed, not negated (osc_bl_parf, SDK index logic) */
static inline float oa_bl_par_lookup(float x, const float *wt)
{
    const float p = x - (float)(uint32_t)x;
    const float x0f = 2.f * p * OA_WT_SIZE;
    const uint32_t x0p = (uint32_t)x0f;
    const uint32_t x0 = (x0p <= OA_WT_SIZE) ? x0p : (OA_WT_SIZE - (x0p & OA_WT_MASK));
    const uint32_t x1 = (x0p < (OA_WT_SIZE - 1)) ? (x0 + 1) & OA_WT_MASK : (x0p >= OA_WT_SIZE) ? (x0 - 1) & OA_WT_MASK : (x0 + 1);
    return oa_linint(x0f - (float)x0p, wt[x0], wt[x1]);
}
static inline float oa_bl_sawf(float x, uint8_t i) { return oa_bl_half(oa_saw, x, i); }
static inline float oa_bl_sqrf(float x, uint8_t i) { return oa_bl_half(oa_sqr, x, i); }
static inline float oa_bl_parf(float x, uint8_t i) { return oa_bl_par_lookup(x, &oa_par[(i < OA_BL_CNT ? i : OA_BL_CNT - 1) * OA_WT_LUT]); }
static inline float oa_bl2_sawf(float x, float i) { return oa_bl2_half(oa_saw, x, i); }
static inline float oa_bl2_sqrf(float x, float i) { return oa_bl2_half(oa_sqr, x, i); }
static inline float oa_bl2_parf(float x, float i)
{
    const uint8_t i0 = (uint8_t)i, i1 = (uint8_t)(i0 + 1 < OA_BL_CNT ? i0 + 1 : OA_BL_CNT - 1);
    return oa_linint(i - (float)i0, oa_bl_parf(x, i0), oa_bl_parf(x, i1));
}

/* fractional table index for a (fractional) note: osc_bl_saw_idx / sqr / par */
float oa_bl_saw_idx(float note);
float oa_bl_sqr_idx(float note);
float oa_bl_par_idx(float note);

float oa_white(void);    /* osc_white: Gaussian white noise in [-1, 1] (SDK doc) */
uint32_t oa_rand(void);  /* osc_rand */
