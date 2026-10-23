/* PrLogueXXL -- effects: MOD FX (Chorus 8, Ensemble 3, Phaser 8, Flanger 8), DELAY (12 types), REVERB (10 types),
 * and the L.F. COMP. Types and subtypes are KORG's (docs/PROLOGUE_SPEC.md, MIDI notes P2-P6, P10, P11); the
 * algorithms are PrLogueXXL's own (KORG publishes none). */
#pragma once
#include "dsp.h"

#define MFX_MAX 4096
typedef struct {
    float *buf[2];
    int w;
    float ph, ph2;
    float ap[2][12], apz[2];
    float fz[2];
    dcb_t dc[2];
} modfx_t;

#define DLY_MAX (3 * 44100)
typedef struct {
    float *buf[2];
    int w;
    float tSm;
    float lp[2], hp[2];
    float wow;
    rng_t rng;
    dcb_t dc[2];
} delay_t;

#define REV_LINES 8
typedef struct {
    float *line[REV_LINES];
    int len[REV_LINES], w[REV_LINES];
    float damp[REV_LINES];
    float *ap[4];
    int apLen[4], apW[4];
    float *pre;
    int preW;
    float modPh, hz;
    float *ps;
    int psW;
    float psPh;
    lp1_t inHp;
    rng_t rng;
    float rnd[2], rndT[2];
} reverb_t;

typedef struct {
    float env, gr, grDb;
    float shZ[2], lpZ;
} lfcomp_t;

int fx_alloc(modfx_t *m, delay_t *d, reverb_t *r);
void fx_free(modfx_t *m, delay_t *d, reverb_t *r);
void fx_clear(modfx_t *m, delay_t *d, reverb_t *r);

/* type 0 chorus 1 ensemble 2 phaser 3 flanger; speed/depth 0..1; in place */
void modfx_process(modfx_t *m, int type, int sub, float speed, float depth, float *L, float *R, int n);
/* delay: type 0..11 (P11), time/depth 0..1, wet 0..1; in place (dry + wet) */
void delay_process(delay_t *d, int type, float time, float depth, float wet, float tempo, float *L, float *R, int n);
void reverb_process(reverb_t *r, int type, float time, float depth, float wet, float *L, float *R, int n);
/* the L.F. COMP: gain 0..1; returns the compression in dB (>= 0) for the meter */
float lfcomp_process(lfcomp_t *c, float gain, float *L, float *R, int n);
