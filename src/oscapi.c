/* PrLogueXXL -- the logue SDK oscillator runtime's tables, computed to the SDK's definitions. See oscapi.h. */
#include "oscapi.h"
#include <math.h>
#include <pthread.h>

float oa_midi_to_hz[OA_MIDI_HZ_SIZE];
float oa_sine[OA_WT_LUT];
float oa_saw[OA_BL_CNT * OA_WT_LUT], oa_sqr[OA_BL_CNT * OA_WT_LUT], oa_par[OA_BL_CNT * OA_WT_LUT];
/* the first note each band-limited table serves (7 tables, as k_wt_*_notes_cnt): one per octave from C2 up */
uint8_t oa_saw_notes[OA_BL_CNT] = {0, 36, 48, 60, 72, 84, 96};
uint8_t oa_sqr_notes[OA_BL_CNT] = {0, 36, 48, 60, 72, 84, 96};
uint8_t oa_par_notes[OA_BL_CNT] = {0, 36, 48, 60, 72, 84, 96};

static pthread_once_t g_once = PTHREAD_ONCE_INIT;

static double note_hz(double n) { return 440.0 * pow(2.0, (n - 69.0) / 12.0); }

/* the highest harmonic that stays under 20 kHz up to the top note a table serves (and under the table's own
 * resolution: 256 points a period hold 127 harmonics) */
static int harmonics(const uint8_t *notes, int t)
{
    const double top = t + 1 < OA_BL_CNT ? notes[t + 1] : 151;
    int h = (int)floor(20000.0 / note_hz(top));
    return h < 1 ? 1 : h > 127 ? 127 : h;
}

static void fill(float *tab, const uint8_t *notes, int kind)
{
    int t, i, k;
    for (t = 0; t < OA_BL_CNT; t++) {
        const int H = harmonics(notes, t);
        double v[OA_WT_LUT], pk = 0;
        for (i = 0; i < OA_WT_LUT; i++) {
            const double x = 0.5 * i / OA_WT_SIZE;   /* the half wave: phase 0 .. 0.5 */
            double s = 0;
            for (k = 1; k <= H; k++) {
                if (kind == 0) s += sin(2 * M_PI * k * x) / k;                              /* saw */
                else if (kind == 1) { if (k & 1) s += sin(2 * M_PI * k * x) / k; }          /* square: odd */
                else s += cos(2 * M_PI * k * x) / ((double)k * k);                          /* parabola */
            }
            v[i] = s;
        }
        if (kind == 2) {   /* the parabola: centred, peak 1 */
            double mn = v[0], mx = v[0];
            for (i = 0; i < OA_WT_LUT; i++) { if (v[i] < mn) mn = v[i]; if (v[i] > mx) mx = v[i]; }
            for (i = 0; i < OA_WT_LUT; i++) tab[t * OA_WT_LUT + i] = (float)(2 * (v[i] - mn) / (mx - mn) - 1);
            continue;
        }
        for (i = 0; i < OA_WT_LUT; i++) if (fabs(v[i]) > pk) pk = fabs(v[i]);
        for (i = 0; i < OA_WT_LUT; i++) tab[t * OA_WT_LUT + i] = (float)(v[i] / pk);
    }
}

static void build(void)
{
    int i;
    for (i = 0; i < OA_MIDI_HZ_SIZE; i++) {
        const double f = note_hz(i);
        oa_midi_to_hz[i] = (float)(f > OA_NOTE_MAX_HZ ? OA_NOTE_MAX_HZ : f);
    }
    for (i = 0; i < OA_WT_LUT; i++) oa_sine[i] = (float)sin(2 * M_PI * 0.5 * i / OA_WT_SIZE);
    fill(oa_saw, oa_saw_notes, 0);
    fill(oa_sqr, oa_sqr_notes, 1);
    fill(oa_par, oa_par_notes, 2);
}

void oa_init(void) { pthread_once(&g_once, build); }

static float bl_idx(const uint8_t *notes, float note)
{
    int t;
    if (note <= notes[0]) return 0;
    for (t = 0; t < OA_BL_CNT - 1; t++)
        if (note < notes[t + 1]) return (float)t + (note - notes[t]) / (float)(notes[t + 1] - notes[t]);
    return OA_BL_CNT - 1;
}
float oa_bl_saw_idx(float note) { return bl_idx(oa_saw_notes, note); }
float oa_bl_sqr_idx(float note) { return bl_idx(oa_sqr_notes, note); }
float oa_bl_par_idx(float note) { return bl_idx(oa_par_notes, note); }

static uint32_t g_rand = 0x2545F491u;
uint32_t oa_rand(void)
{
    uint32_t x = g_rand;   /* xorshift32; shared state, races only reorder noise */
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return g_rand = x ? x : 0x2545F491u;
}

float oa_white(void)
{
    /* Gaussian (sum of four uniforms, unit variance-ish), clipped to [-1, 1] as the SDK documents */
    float s = 0;
    int i;
    for (i = 0; i < 4; i++) s += (float)(oa_rand() >> 8) * (1.f / 16777216.f) - 0.5f;
    s *= 0.866f;
    return s > 1.f ? 1.f : s < -1.f ? -1.f : s;
}
