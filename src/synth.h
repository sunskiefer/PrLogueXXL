/* PrLogueXXL -- the synth: voices, voice modes, timbres, arpeggiator, effects, L.F. COMP. */
#pragma once
#include "pl.h"
#include "fx.h"

/* a timbre's values for one block, derived from its program fields + modulation (wheel, aftertouch) */
typedef struct {
    int v1wave, v2wave, syncRing, pegTgt, mType, selNoise, selVpm, selUser, mRoute, drive, lowcut;
    int lfoWave, lfoMode, lfoTgt, lfoTosc, vmType, legato, lfoKsync, lfoVsync, keytrk;
    float v1semi, v2semi, msemi, v1cents, v2cents, v1shape, v2shape, xmod, pegCents;
    float lv1, lv2, lvm, cutoffOct, reso, cutEg, cutVel, ampVel;
    float aA, aD, aS, aR, eA, eD, eS, eR;          /* seconds / level */
    float lfoHz, lfoInt, mShape, mShift, porta, spread, vmDepth;
    float vpm[6], usr[6];
    float bendUp, bendDn;
} tparams_t;

/* VPM type definitions (docs/PROLOGUE_SPEC.md: KORG's descriptions + the ratio table supplied by the user) */
typedef struct {
    int carrier;        /* 0 sine, 1 saw, 2 square, 3 pseudo saw (feedback sine), 4 pseudo square, 5 driven sine */
    int ratioSet;       /* 0 full list (1:4 1:2 1:1 2:1 .. 15:1), 1 from 2:1, 2 +3 harmonics, 3 +4 (5:1 base), 4 sub-harmonics,
                         * 5 sub-harmonic offsets, 6 from 9:1 */
    float fb, noise, envInt, att, dec, drive;
    int evolve;         /* 1 creep, 2 throat */
} vpmdef_t;
extern const vpmdef_t VPM_DEF[16];
float vpm_ratio(int set, float shift01);

void voice_note_on(voice_t *v, const tparams_t *tp, int timbre, int note, float vel, float transpose, float detune,
                   float level, float pan, int legato, uint32_t age);
void voice_note_off(voice_t *v);
void voice_kill(voice_t *v);
/* adds the voice into L/R (n frames); mod: pitch bend semitones, shared LFO value (voice sync), tempo for BPM LFO */
void voice_block(voice_t *v, const tparams_t *tp, float bendSemi, float sharedLfo, float tempo, float tune,
                 float *L, float *R, int n);
float lfo_wave(int wave, float ph, float *hold, rng_t *r);
void user_osc_param_all(int slot, int index, int value);   /* user.cpp: OSC_PARAM to every voice's instance */
void user_osc_init(void);
