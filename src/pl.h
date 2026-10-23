/* PrLogueXXL -- a KORG prologue-16 style polyphonic synthesizer for MPC OS (mpc-vst-plugins).
 *
 * Structure follows the prologue (docs/PROLOGUE_SPEC.md): a program = common settings + two timbres (Main, Sub);
 * a timbre = VCO1, VCO2, MULTI ENGINE (NOISE / VPM / USER), mixer, VCF, AMP EG, EG, LFO, voice mode. 16 voices
 * (8 + 8 with the sub timbre on). Effects: MOD FX, DELAY/REVERB, then the L.F. COMP. Values are stored in KORG's
 * own ranges (10-bit knobs 0..1023, KORG's enumerations), so the value curves of the MIDI implementation apply. */
#pragma once
#include <stdint.h>
#include "dsp.h"

#define PL_VOICES 16
#define PL_BLOCK 128          /* engine block; control rate = every CTRL frames */
#define PL_CTRL 16
#define PL_PROGRAMS 500
#define PL_NAME 12

/* ---- timbre fields (KORG timbre block, docs/PROLOGUE_SPEC.md) */
enum {
    T_PORTA, T_SPREAD, T_VM_DEPTH, T_VM_TYPE,
    T_V1_WAVE, T_V1_OCT, T_V1_PITCH, T_V1_SHAPE, T_PEG_TGT, T_PEG_INT,
    T_V2_WAVE, T_V2_OCT, T_V2_PITCH, T_V2_SHAPE, T_SYNC_RING, T_XMOD,
    T_M_ROUTE, T_M_TYPE, T_M_OCT, T_SEL_NOISE, T_SEL_VPM, T_SEL_USER, T_SH_NOISE, T_SH_VPM, T_SSH_VPM, T_SH_USER, T_SSH_USER,
    T_LV1, T_LV2, T_LVM,
    T_CUTOFF, T_RESO, T_CUT_EG, T_DRIVE, T_LOWCUT, T_KEYTRK, T_CUT_VEL,
    T_AA, T_AD, T_AS, T_AR, T_EA, T_ED, T_ES, T_ER,
    T_LFO_WAVE, T_LFO_MODE, T_LFO_RATE, T_LFO_INT, T_LFO_TGT,
    T_MW_ASSIGN, T_BEND_UP, T_BEND_DN,
    T_VPM_P1, T_VPM_P2, T_VPM_P3, T_VPM_P4, T_VPM_P5, T_VPM_P6,
    T_USR_P1, T_USR_P2, T_USR_P3, T_USR_P4, T_USR_P5, T_USR_P6,
    T_MW_RANGE, T_LFO_KSYNC, T_LFO_VSYNC, T_LFO_TOSC, T_LEGATO, T_AT_ASSIGN,
    T_COUNT
};
/* ---- program common fields */
enum {
    C_OCTAVE, C_SUB_ON, C_EDIT_TIMBRE, C_TIMBRE_TYPE, C_BALANCE, C_POSITION, C_SPLIT, C_TEMPO, C_ARP_TGT, C_AMP_VEL,
    C_PORTA_MODE, C_LEVEL, C_MFX_TYPE, C_MFX_SPEED, C_MFX_DEPTH, C_MFX_CHO, C_MFX_ENS, C_MFX_PHA, C_MFX_FLA,
    C_TUNING, C_SCALE_KEY, C_PRG_TUNE, C_TRANSPOSE, C_ARP_GATE, C_ARP_RATE, C_DRYWET, C_DR_TYPE, C_DR_TIME, C_DR_DEPTH,
    C_REV_TYPE, C_DLY_TYPE, C_MFX_ROUTE, C_DR_ROUTE, C_MFX_ON, C_DR_ON, C_ARP_MODE, C_ARP_RANGE, C_ARP_TYPE, C_CATEGORY,
    C_COUNT
};

typedef struct { const char *key, *name; int16_t min, max, def; } field_t;
extern const field_t T_FIELDS[T_COUNT];
extern const field_t C_FIELDS[C_COUNT];

typedef struct {
    char name[PL_NAME + 1];
    int16_t c[C_COUNT];
    int16_t t[2][T_COUNT];
} program_t;

void prog_init(program_t *p, const char *name);   /* KORG-like "Init Program" */
int prog_factory_count(void);
void prog_factory(program_t *p, int n);           /* PrLogueXXL's own factory programs */

/* ---- value curves from KORG's MIDI implementation (docs/PROLOGUE_SPEC.md) */
float pl_pitch_cents(int v);        /* note P16: VCO pitch 0..1023 -> -1200..1200 cent */
float pl_peg_cents(int v);          /* note P17: pitch EG int -> -4800..4800 cent */
float pl_cut_eg_pct(int v);         /* note P20: cutoff EG int -> -100..100 % */
int pl_chord_index(int depth);      /* note P13: chord bands */
extern const char *const CHORD_NAMES[14];

/* ---- one voice */
typedef enum { EG_IDLE, EG_ATT, EG_DEC, EG_SUS, EG_REL } egst_t;
typedef struct { egst_t st; float v, relFrom; } eg_t;

typedef struct {
    int idx, active, timbre, note, gate, stolenFade;
    float vel;              /* 0..1 after the velocity curve */
    float pitch, target;    /* note number with glide */
    float detune;           /* cents (duo / unison / analog spread) */
    float transpose;        /* semitones (mono sub voices, chord notes) */
    float level, pan;
    uint32_t age;
    /* oscillators */
    double ph1, ph2;
    float tri1, tri2, last1;
    float drift, driftT;
    /* multi engine */
    double phm, phmod;
    float fbPrev, vpmEnv, decimHold, decimPh;
    svf_t nf;
    rng_t rng;
    /* filter */
    svf_t f;
    float lcz, cutJit;
    eg_t ea, ef;
    float lfoPh, lfoHold;
    float out;
} voice_t;

/* ---- the whole synth */
typedef struct synth synth_t;
int synth_init(synth_t **s, const char *dataDir);
void synth_free(synth_t *s);
