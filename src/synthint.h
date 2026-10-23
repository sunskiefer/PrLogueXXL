/* PrLogueXXL -- the synth's internal state (synth.c, panel.c, engine.c). */
#pragma once
#include "synth.h"
#include "param_ids.h"

typedef struct { int8_t keys[16]; int n; } monostack_t;

typedef struct {
    int8_t held[32];
    int n, pos, physical, latchedUp;
    int sounding, sounding2;
    float phase, stepFrames, gateLeft;
    double lastHostStep;
} arp_t;

typedef struct {
    int masterTune, transpose, velCurve;   /* GLOBAL */
    int lfcOn;
    float lfcGain, master;
} globals_t;

/* the panel's state machine (panel.c) */
enum { CTX_NONE, CTX_MFX, CTX_DLRV, CTX_TEMPO, CTX_MULTI };
typedef struct {
    int edit, global, page, item;          /* PROGRAM EDIT / GLOBAL EDIT menus */
    int shift;
    int ctx;                               /* what the PROGRAM/VALUE knob edits outside the menus */
    uint32_t ctxUntil, msgUntil;
    char msg1[40], msg2[40];
    float valueAcc, lastValue;
    uint32_t lastValueClock;
    int fav[8];
    int dirty;                             /* program edited since load */
} panel_t;

struct synth {
    voice_t v[PL_VOICES];
    tparams_t tp[2];
    program_t prog;                        /* the program being played/edited */
    int progNum;
    program_t *bank;                       /* PL_PROGRAMS */
    uint8_t written[PL_PROGRAMS];          /* user-written slots (saved to the programs file) */
    globals_t g;
    float bend, modwheel, aftertouch, cOff[C_COUNT], mwShape[2];
    int damper, bankLsb;
    uint8_t sustained[128], keyDown[128];
    monostack_t mono[2];
    arp_t arp;
    float lfoPh[2];
    uint32_t ageCounter, clock;
    rng_t rng;
    modfx_t mfx;
    delay_t dly;
    reverb_t rev;
    lfcomp_t lfc;
    float vu;
    dcb_t dc[2][2];
    struct { int playing; double ppq, tempo; } host;
    panel_t ui;
    float pv[P_COUNT];                     /* the plugin parameters' values (real units) */
    uint32_t rev_;                         /* display revision (HAS_DISPLAY_REV) */
    char dataDir[512];
};

void synth_derive(synth_t *s, int t);
void synth_render(synth_t *s, float *L, float *R, int n);
void synth_note_on(synth_t *s, int note, int vel);
void synth_note_off(synth_t *s, int note);
void synth_damper(synth_t *s, int on);
void synth_all_off(synth_t *s, int hard);
float synth_tempo(const synth_t *s);
float tsec_att(float x);
float tsec_dr(float x);
void mono_push(monostack_t *m, int key);
void mono_pop(monostack_t *m, int key);
void arp_key(synth_t *s, int key, int down);
extern const char *const TUNING_NAMES[6];
extern const char *const MW_NAMES[32];

/* panel.c */
void panel_init(synth_t *s);
void panel_set(synth_t *s, int p, float v);              /* a plugin parameter moved (host / touch / Q-Link) */
void panel_sync(synth_t *s);                              /* program -> panel parameter values */
void panel_tick(synth_t *s);                              /* once per block: timers, meter */
int panel_text(synth_t *s, int p, char *buf, int n);     /* "<key>_display" */
void panel_oled(synth_t *s, int line, char *buf, int n);
int panel_led(synth_t *s, int p);                         /* "<key>_on" */
void load_program(synth_t *s, int n);
char *state_save(synth_t *s, int *len);
int state_load(synth_t *s, const char *txt);
void bank_load(synth_t *s);
void bank_save(synth_t *s);
void program_name(synth_t *s, int n, char *buf, int len);
