/* PrLogueXXL -- effects. See fx.h. Algorithms and value curves are PrLogueXXL's own. */
#include <stdlib.h>
#include <string.h>
#include "fx.h"

int fx_alloc(modfx_t *m, delay_t *d, reverb_t *r)
{
    static const int LEN[REV_LINES] = {1913, 2251, 2569, 2879, 3221, 3557, 3889, 4307};
    static const int AP[4] = {131, 97, 347, 251};
    int i;
    memset(m, 0, sizeof *m); memset(d, 0, sizeof *d); memset(r, 0, sizeof *r);
    m->buf[0] = calloc(MFX_MAX, sizeof(float)); m->buf[1] = calloc(MFX_MAX, sizeof(float));
    d->buf[0] = calloc(DLY_MAX, sizeof(float)); d->buf[1] = calloc(DLY_MAX, sizeof(float));
    d->rng.s = 0xDE1A7u; r->rng.s = 0xBEEF1u;
    for (i = 0; i < REV_LINES; i++) { r->len[i] = LEN[i] * 3; r->line[i] = calloc((size_t)r->len[i], sizeof(float)); if (!r->line[i]) return -1; }
    for (i = 0; i < 4; i++) { r->apLen[i] = AP[i]; r->ap[i] = calloc((size_t)AP[i], sizeof(float)); if (!r->ap[i]) return -1; }
    r->ps = calloc(4096, sizeof(float));
    r->pre = calloc(8192, sizeof(float));
    if (!m->buf[0] || !m->buf[1] || !d->buf[0] || !d->buf[1] || !r->ps || !r->pre) return -1;
    return 0;
}

void fx_free(modfx_t *m, delay_t *d, reverb_t *r)
{
    int i;
    free(m->buf[0]); free(m->buf[1]); free(d->buf[0]); free(d->buf[1]);
    for (i = 0; i < REV_LINES; i++) free(r->line[i]);
    for (i = 0; i < 4; i++) free(r->ap[i]);
    free(r->ps); free(r->pre);
}

void fx_clear(modfx_t *m, delay_t *d, reverb_t *r)
{
    int i;
    if (m->buf[0]) { memset(m->buf[0], 0, MFX_MAX * sizeof(float)); memset(m->buf[1], 0, MFX_MAX * sizeof(float)); }
    if (d->buf[0]) { memset(d->buf[0], 0, DLY_MAX * sizeof(float)); memset(d->buf[1], 0, DLY_MAX * sizeof(float)); }
    for (i = 0; i < REV_LINES; i++) if (r->line[i]) memset(r->line[i], 0, (size_t)r->len[i] * sizeof(float));
    for (i = 0; i < 4; i++) if (r->ap[i]) memset(r->ap[i], 0, (size_t)r->apLen[i] * sizeof(float));
    if (r->ps) memset(r->ps, 0, 4096 * sizeof(float));
    if (r->pre) memset(r->pre, 0, 8192 * sizeof(float));
    memset(m->ap, 0, sizeof m->ap);
}

static inline float rd(const float *b, int size, int w, float t)
{
    float p = (float)w - t;
    int i0;
    float fr;
    while (p < 0) p += (float)size;
    i0 = (int)p;
    fr = p - (float)i0;
    i0 %= size;
    return b[i0] + fr * (b[(i0 + 1) % size] - b[i0]);
}

static inline float tri01(float ph) { ph -= floorf(ph); return ph < 0.5f ? 4.f * ph - 1.f : 3.f - 4.f * ph; }

/* ---------------------------------------------------------------- MOD FX ---- */
typedef struct { float base, depth, mix, fb, phase, rateMul; int taps, mono, vib, bright; } chorusdef_t;
static const chorusdef_t CHO[8] = {
    /* base ms, depth ms, mix, fb, R phase, rate, taps, mono, vibrato, harmonic */
    {12.f, 4.0f, 0.5f, 0.0f, 0.25f, 1.f, 1, 0, 0, 0},    /* STEREO */
    {10.f, 2.0f, 0.35f, 0.0f, 0.25f, 1.f, 1, 0, 0, 0},   /* LIGHT */
    {16.f, 7.0f, 0.55f, 0.1f, 0.25f, 0.8f, 1, 0, 0, 0},  /* DEEP */
    {11.f, 3.5f, 0.5f, 0.0f, 0.333f, 1.f, 3, 0, 0, 0},   /* TRIPHASE */
    {9.f, 3.0f, 0.5f, 0.0f, 0.25f, 1.f, 1, 0, 0, 1},     /* HARMONIC */
    {12.f, 4.0f, 0.5f, 0.0f, 0.0f, 1.f, 1, 1, 0, 0},     /* MONO */
    {7.f, 3.0f, 0.5f, 0.55f, 0.25f, 1.f, 1, 0, 0, 0},    /* FEEDBACK */
    {6.f, 3.0f, 1.0f, 0.0f, 0.0f, 2.f, 1, 1, 1, 0},      /* VIBRATO */
};
typedef struct { int stages; float fmin, fmax, fb, rateMul, phase, mix; } phaserdef_t;
static const phaserdef_t PHA[8] = {
    {6, 200.f, 4000.f, 0.4f, 1.f, 0.25f, 0.5f},     /* STEREO */
    {6, 200.f, 4000.f, 0.4f, 3.f, 0.25f, 0.5f},     /* FAST */
    {4, 300.f, 2500.f, 0.2f, 1.f, 0.0f, 0.5f},      /* ORANGE (4-stage orange-box style) */
    {4, 250.f, 2200.f, 0.0f, 1.f, 0.0f, 0.5f},      /* SMALL */
    {4, 250.f, 2200.f, 0.7f, 1.f, 0.0f, 0.5f},      /* SMALL RESO */
    {8, 120.f, 6000.f, 0.5f, 0.8f, 0.0f, 0.5f},     /* BLACK (wide range) */
    {12, 400.f, 1800.f, 0.6f, 0.7f, 0.15f, 0.55f},  /* FORMANT */
    {8, 1500.f, 9000.f, 0.6f, 2.f, 0.5f, 0.5f},     /* TWINKLE */
};
typedef struct { float base, depth, fb, mix, phase, rateMul; int mono, pan, taps; } flangerdef_t;
static const flangerdef_t FLA[8] = {
    {2.0f, 1.8f, 0.6f, 0.5f, 0.25f, 1.f, 0, 0, 1},   /* STEREO */
    {1.5f, 1.2f, 0.3f, 0.4f, 0.25f, 1.f, 0, 0, 1},   /* LIGHT */
    {2.0f, 1.8f, 0.6f, 0.5f, 0.0f, 1.f, 1, 0, 1},    /* MONO */
    {0.6f, 0.5f, 0.7f, 0.5f, 0.25f, 1.f, 0, 0, 1},   /* HIGH SWEEP */
    {1.6f, 1.4f, 0.7f, 0.5f, 0.25f, 1.f, 0, 0, 1},   /* MID SWEEP */
    {2.0f, 1.8f, 0.6f, 0.5f, 0.5f, 1.f, 0, 1, 1},    /* PAN SWEEP */
    {2.0f, 1.8f, 0.7f, 0.5f, 0.0f, 1.f, 1, 0, 1},    /* MONO SWEEP */
    {2.0f, 1.6f, 0.5f, 0.5f, 0.333f, 1.f, 0, 0, 3},  /* TRIPHASE */
};

void modfx_process(modfx_t *m, int type, int sub, float speed, float depth, float *L, float *R, int n)
{
    int i;
    if (!m->buf[0]) return;
    sub &= 7;
    if (type == 0 || type == 1 || type == 3) {
        float base, dep, mix, fb, phase, rate;
        int taps, mono, vib, pan = 0, bright = 0;
        if (type == 0) { const chorusdef_t *c = &CHO[sub]; base = c->base; dep = c->depth; mix = c->mix; fb = c->fb; phase = c->phase; rate = c->rateMul; taps = c->taps; mono = c->mono; vib = c->vib; bright = c->bright; }
        else if (type == 1) {   /* ENSEMBLE: STEREO LIGHT MONO, three taps 120 degrees apart + a fast shimmer */
            const int s = sub > 2 ? 0 : sub;
            base = 9.f; dep = s == 1 ? 1.6f : 2.8f; mix = s == 1 ? 0.4f : 0.55f; fb = 0; phase = s == 2 ? 0.f : 0.166f;
            rate = 1.f; taps = 3; mono = s == 2; vib = 0;
        } else { const flangerdef_t *f = &FLA[sub]; base = f->base; dep = f->depth; mix = f->mix; fb = f->fb; phase = f->phase; rate = f->rateMul; taps = f->taps; mono = f->mono; vib = 0; pan = f->pan; }
        {
            const float hz = 0.05f * powf(160.f, speed) * rate;   /* 0.05 .. 8 Hz */
            const float dd = dep * (0.15f + 0.85f * depth) * SR / 1000.f;
            const float bb = base * SR / 1000.f;
            for (i = 0; i < n; i++) {
                float inL = L[i], inR = R[i], yl = 0, yr = 0, mono_in = (inL + inR) * 0.5f;
                int t;
                m->ph += hz / SR; if (m->ph >= 1.f) m->ph -= 1.f;
                m->ph2 += 6.1f / SR; if (m->ph2 >= 1.f) m->ph2 -= 1.f;
                for (t = 0; t < taps; t++) {
                    const float off = (float)t / (float)taps;
                    float ml = tri01(m->ph + off), mr = tri01(m->ph + off + phase);
                    if (type == 1) { ml += 0.12f * fsin_ph(m->ph2 + off); mr += 0.12f * fsin_ph(m->ph2 + off + 0.25f); }
                    yl += rd(m->buf[0], MFX_MAX, m->w, bb + dd * (1.f + ml) * 0.5f + 1.f);
                    yr += rd(m->buf[mono ? 0 : 1], MFX_MAX, m->w, bb + dd * (1.f + mr) * 0.5f + 1.f);
                }
                yl /= (float)taps; yr /= (float)taps;
                if (bright) { m->fz[0] += 0.2f * (yl - m->fz[0]); m->fz[1] += 0.2f * (yr - m->fz[1]); yl += (yl - m->fz[0]) * 0.8f; yr += (yr - m->fz[1]) * 0.8f; }
                m->buf[0][m->w] = flush((mono ? mono_in : inL) + yl * fb);
                m->buf[1][m->w] = flush(inR + yr * fb);
                if (++m->w >= MFX_MAX) m->w = 0;
                if (pan) { const float p = tri01(m->ph); yl *= 0.5f + 0.5f * p; yr *= 0.5f - 0.5f * p; }
                if (vib) { L[i] = yl; R[i] = mono ? yl : yr; }
                else { L[i] = inL * (1.f - mix * 0.5f) + yl * mix; R[i] = inR * (1.f - mix * 0.5f) + (mono ? yl : yr) * mix; }
            }
        }
    } else if (type == 2) {
        const phaserdef_t *p = &PHA[sub];
        const float hz = 0.03f * powf(200.f, speed) * p->rateMul;
        for (i = 0; i < n; i++) {
            int ch;
            m->ph += hz / SR; if (m->ph >= 1.f) m->ph -= 1.f;
            for (ch = 0; ch < 2; ch++) {
                const float lfo = 0.5f + 0.5f * tri01(m->ph + (ch ? p->phase : 0.f));
                const float f = p->fmin * powf(p->fmax / p->fmin, lfo * (0.2f + 0.8f * depth));
                const float t = tanf((float)M_PI * clampf(f, 20.f, 18000.f) / SR), a = (t - 1.f) / (t + 1.f);
                float x = (ch ? R[i] : L[i]) + m->apz[ch] * p->fb, y = x;
                int s;
                for (s = 0; s < p->stages; s++) {
                    const float o = a * y + m->ap[ch][s];
                    m->ap[ch][s] = flush(y - a * o);
                    y = o;
                }
                m->apz[ch] = flush(y);
                if (ch) R[i] = R[i] * (1.f - p->mix) + y * p->mix * 1.1f; else L[i] = L[i] * (1.f - p->mix) + y * p->mix * 1.1f;
            }
        }
    }
}

/* ---------------------------------------------------------------- DELAY ---- */
void delay_process(delay_t *d, int type, float time, float depth, float wet, float tempo, float *L, float *R, int n)
{
    /* BPM divisions in beats (OWN list, 1/32 note .. 1 bar) */
    static const float DIV[14] = {0.125f, 0.1667f, 0.25f, 0.333f, 0.375f, 0.5f, 0.667f, 0.75f, 1.f, 1.333f, 1.5f, 2.f, 3.f, 4.f};
    const int bpm = type >= 6 && type <= 10, base = bpm ? type - 6 : type;   /* 0 stereo 1 mono 2 ping 3 hipass 4 tape 5 one tap 11 doubling */
    float tSec, fb, dryG, wetG;
    int i;
    if (!d->buf[0]) return;
    if (type == 11) tSec = 0.012f + 0.05f * time;
    else if (bpm) tSec = DIV[(int)clampf(time * 13.99f, 0, 13)] * 60.f / (tempo > 1 ? tempo : 120.f);
    else tSec = 0.01f * powf(140.f, time);   /* 10 ms .. 1.4 s */
    if (tSec > (DLY_MAX - 64) / SR) tSec = (DLY_MAX - 64) / SR;
    fb = base == 5 || type == 11 ? 0.f : depth * 0.93f;   /* "a little under unity" at the top (Sound On Sound) */
    dryG = wet < 0.5f ? 1.f : 2.f * (1.f - wet);
    wetG = wet > 0.5f ? 1.f : 2.f * wet;
    if (base == 5) wetG *= 0.4f + 0.6f * depth;
    for (i = 0; i < n; i++) {
        float t = tSec * SR, yl, yr, xl, xr;
        d->tSm += (t - d->tSm) * 0.0006f;
        if (d->tSm < 1) d->tSm = 1;
        t = d->tSm;
        if (base == 4) {   /* tape: wow + flutter */
            d->wow += 0.6f / SR; if (d->wow >= 1.f) d->wow -= 1.f;
            t += (0.0018f * fsin_ph(d->wow) + 0.0004f * rng_f(&d->rng)) * SR;
        }
        if (type == 11) { yl = rd(d->buf[0], DLY_MAX, d->w, t); yr = rd(d->buf[1], DLY_MAX, d->w, t * (1.f + 0.4f * depth)); }
        else { yl = rd(d->buf[0], DLY_MAX, d->w, t); yr = rd(d->buf[1], DLY_MAX, d->w, base == 0 ? t * 1.07f : t); }
        xl = yl * fb; xr = yr * fb;
        if (base == 3) { d->hp[0] += 0.06f * (xl - d->hp[0]); d->hp[1] += 0.06f * (xr - d->hp[1]); xl -= d->hp[0]; xr -= d->hp[1]; }
        if (base == 4) { d->lp[0] += 0.25f * (xl - d->lp[0]); d->lp[1] += 0.25f * (xr - d->lp[1]); xl = sat(d->lp[0] * 1.2f); xr = sat(d->lp[1] * 1.2f); }
        else { d->lp[0] += 0.5f * (xl - d->lp[0]); d->lp[1] += 0.5f * (xr - d->lp[1]); xl = d->lp[0]; xr = d->lp[1]; }
        xl = dcb_tick(&d->dc[0], xl); xr = dcb_tick(&d->dc[1], xr);
        if (base == 1) { d->buf[0][d->w] = flush((L[i] + R[i]) * 0.5f + xl); d->buf[1][d->w] = d->buf[0][d->w]; yr = yl; }
        else if (base == 2) { d->buf[0][d->w] = flush((L[i] + R[i]) * 0.5f + xr); d->buf[1][d->w] = flush(xl); }
        else { d->buf[0][d->w] = flush(L[i] + xl); d->buf[1][d->w] = flush(R[i] + xr); }
        if (++d->w >= DLY_MAX) d->w = 0;
        L[i] = L[i] * dryG + yl * wetG;
        R[i] = R[i] * dryG + yr * wetG;
    }
}

/* ---------------------------------------------------------------- REVERB ---- */
typedef struct { float size, rtMax, bright, pre, mod; int shift; float early; } revdef_t;
static const revdef_t REV[10] = {
    {0.75f, 6.f, 5000.f, 0.020f, 1.f, 0, 0.3f},    /* HALL */
    {0.65f, 8.f, 9000.f, 0.010f, 0.5f, 0, 0.1f},   /* SMOOTH: clear, sustained highs */
    {1.0f, 12.f, 4000.f, 0.040f, 1.f, 0, 0.3f},    /* ARENA */
    {0.45f, 5.f, 7000.f, 0.000f, 0.3f, 0, 0.0f},   /* PLATE */
    {0.30f, 2.f, 4500.f, 0.005f, 0.5f, 0, 0.5f},   /* ROOM */
    {0.25f, 0.6f, 6000.f, 0.008f, 0.f, 0, 1.0f},   /* EARLY REF */
    {0.9f, 16.f, 4500.f, 0.030f, 6.f, 0, 0.2f},    /* SPACE: unstable */
    {0.8f, 10.f, 6000.f, 0.020f, 1.f, 1, 0.1f},    /* RISER: an octave up */
    {0.8f, 10.f, 3000.f, 0.020f, 1.f, 2, 0.1f},    /* SUBMARINE: an octave down */
    {0.7f, 9.f, 3500.f, 0.020f, 12.f, 3, 0.2f},    /* HORROR: very unstable */
};

void reverb_process(reverb_t *r, int type, float time, float depth, float wet, float *L, float *R, int n)
{
    const revdef_t *d = &REV[type < 0 ? 0 : type > 9 ? 9 : type];
    const float rt = 0.2f + (d->rtMax - 0.2f) * powf(time, 1.5f);
    const float inG = 0.25f + 0.75f * depth;
    const float ratio = d->shift == 1 ? 2.f : d->shift == 2 ? 0.5f : 1.f;
    const int preN = (int)(d->pre * SR);
    float g[REV_LINES], dampA, dryG = wet < 0.5f ? 1.f : 2.f * (1.f - wet), wetG = wet > 0.5f ? 1.f : 2.f * wet;
    int Ln[REV_LINES], i, k;
    if (!r->line[0]) return;
    dampA = 1.f - expf(-2.f * (float)M_PI * d->bright / SR);
    for (k = 0; k < REV_LINES; k++) {
        Ln[k] = (int)(r->len[k] * d->size);
        if (Ln[k] < 64) Ln[k] = 64;
        if (Ln[k] > r->len[k] - 40) Ln[k] = r->len[k] - 40;
        g[k] = powf(10.f, -3.f * (float)Ln[k] / SR / rt);
    }
    lp1_set(&r->inHp, 70.f);
    for (i = 0; i < n; i++) {
        float x, o[REV_LINES], s = 0, yl = 0, yr = 0;
        r->pre[r->preW] = (L[i] + R[i]) * 0.5f * inG;
        x = r->pre[(r->preW - preN + 8192) & 8191];
        r->preW = (r->preW + 1) & 8191;
        x -= lp1_tick(&r->inHp, x);
        for (k = 0; k < 4; k++) {
            const float b = r->ap[k][r->apW[k]], y = -0.6f * x + b;
            r->ap[k][r->apW[k]] = x + 0.6f * y;
            if (++r->apW[k] >= r->apLen[k]) r->apW[k] = 0;
            x = y;
        }
        r->modPh += 0.31f / SR; if (r->modPh >= 1.f) r->modPh -= 1.f;
        if (d->shift == 3 && (r->w[0] & 2047) == 0) { r->rndT[0] = rng_f(&r->rng); r->rndT[1] = rng_f(&r->rng); }
        r->rnd[0] += (r->rndT[0] - r->rnd[0]) * 0.0005f; r->rnd[1] += (r->rndT[1] - r->rnd[1]) * 0.0005f;
        for (k = 0; k < REV_LINES; k++) {
            float rp = (float)Ln[k];
            int ri;
            if (k < 4) rp += d->mod * 6.f * (1.f + fsin_ph(r->modPh * (1.f + k * 0.21f) + k * 0.25f)) + (d->shift == 3 ? 30.f * r->rnd[k & 1] : 0.f);
            ri = r->w[k] - (int)rp;
            while (ri < 0) ri += r->len[k];
            o[k] = r->line[k][ri % r->len[k]];
            r->damp[k] += dampA * (o[k] - r->damp[k]);
            r->damp[k] = flush(r->damp[k]);
            o[k] = r->damp[k] * g[k];
            s += o[k];
        }
        s *= 2.f / REV_LINES;
        {
            float fbIn = x;
            if (ratio != 1.f) {
                const float win = 2048.f;
                float p1, p2, a1, a2, ps;
                int i1, i2;
                r->ps[r->psW] = (o[0] + o[3]) * 0.5f;
                r->psPh += (1.f - ratio) / win;
                r->psPh -= floorf(r->psPh);
                p1 = r->psPh * win; p2 = fmodf(p1 + win * 0.5f, win);
                i1 = (r->psW - (int)p1 - 1 + 4096) & 4095;
                i2 = (r->psW - (int)p2 - 1 + 4096) & 4095;
                a1 = fsin_ph(0.5f * p1 / win); a2 = fsin_ph(0.5f * p2 / win);
                ps = r->ps[i1] * a1 * a1 + r->ps[i2] * a2 * a2;
                r->psW = (r->psW + 1) & 4095;
                fbIn += ps * 0.55f;
            }
            for (k = 0; k < REV_LINES; k++) {
                r->line[k][r->w[k]] = flush(o[k] - s + fbIn * (k & 1 ? -1.f : 1.f));
                if (++r->w[k] >= r->len[k]) r->w[k] = 0;
            }
        }
        for (k = 0; k < REV_LINES; k++) { if (k & 1) yr += o[k]; else yl += o[k]; }
        if (d->early > 0) { yl += x * d->early * 0.8f; yr -= x * d->early * 0.8f; }
        L[i] = L[i] * dryG + yl * 0.45f * wetG;
        R[i] = R[i] * dryG + yr * 0.45f * wetG;
    }
}

/* ---------------------------------------------------------------- L.F. COMP ---- */
/* KORG: an analog low-end booster/compressor at the end of the chain; GAIN lowers the threshold and raises the gain;
 * attack, release and ratio are fixed in the circuit and not published. Values here are PrLogueXXL's own. */
float lfcomp_process(lfcomp_t *c, float gain, float *L, float *R, int n)
{
    const float thrDb = -6.f - 24.f * gain, ratio = 4.f, makeDb = 10.f * gain, boost = 0.9f * gain;
    const float atk = expf(-1.f / (0.006f * SR)), rel = expf(-1.f / (0.15f * SR));
    const float shA = 1.f - expf(-2.f * (float)M_PI * 110.f / SR);
    int i;
    for (i = 0; i < n; i++) {
        float l = L[i], r = R[i], lv, db, gr, g;
        /* the low-end boost: a low shelf around 110 Hz */
        c->shZ[0] += shA * (l - c->shZ[0]); c->shZ[1] += shA * (r - c->shZ[1]);
        l += c->shZ[0] * boost; r += c->shZ[1] * boost;
        lv = fabsf(l) > fabsf(r) ? fabsf(l) : fabsf(r);
        c->env = lv > c->env ? atk * c->env + (1.f - atk) * lv : rel * c->env + (1.f - rel) * lv;
        db = 20.f * log10f(c->env + 1e-9f);
        gr = db > thrDb ? (db - thrDb) * (1.f - 1.f / ratio) : 0.f;
        c->grDb = gr;
        g = powf(10.f, (makeDb - gr) / 20.f);
        L[i] = sat(l * g);
        R[i] = sat(r * g);
    }
    return c->grDb;
}
