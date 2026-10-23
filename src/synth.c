/* PrLogueXXL -- the synth: programs and timbres, voice modes, voice allocation, arpeggiator, effects chain,
 * L.F. COMP, and the derived per-block timbre values. The panel and its screens are in panel.c. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "synthint.h"

/* ---------------------------------------------------------------- tuning ---- */
/* micro tunings, cents from equal temperament for C..B (well-known definitions) */
static const float MICRO[6][12] = {
    {0},
    {0, 11.73f, 3.91f, 15.64f, -13.69f, -1.96f, -9.78f, 1.96f, 13.69f, -15.64f, 17.60f, -11.73f},   /* Pure Major (5-limit) */
    {0, 11.73f, 3.91f, 15.64f, -13.69f, -1.96f, -9.78f, 1.96f, 13.69f, -15.64f, 17.60f, -11.73f},   /* Pure Minor */
    {0, -9.78f, 3.91f, -5.87f, 7.82f, -1.96f, 11.73f, 1.96f, -7.82f, 5.87f, -3.91f, 9.78f},        /* Pythagorean */
    {0, -9.78f, -7.82f, -5.87f, -9.78f, -1.96f, -11.73f, -3.91f, -7.82f, -11.73f, -3.91f, -7.82f}, /* Werckmeister III */
    {0, -9.78f, -6.84f, -5.87f, -13.69f, -1.96f, -9.78f, -3.42f, -7.82f, -10.26f, -3.91f, -11.73f},/* Kirnberger III */
};
const char *const TUNING_NAMES[6] = {"Equal Temp", "Pure Major", "Pure Minor", "Pythagorean", "Werckmeister", "Kirnburger"};

static float tune_cents(const synth_t *s, int note)
{
    const int t = s->prog.c[C_TUNING];
    float c = (float)s->g.masterTune + (float)s->prog.c[C_PRG_TUNE];
    if (t > 0 && t < 6) c += MICRO[t][((note - s->prog.c[C_SCALE_KEY]) % 12 + 12) % 12];
    return c;
}

/* velocity curves (global): KORG describes them only in words (type 1 strong touch .. type 6 light touch, 7/8 steady
 * at medium touch, Const127); the shapes are PrLogueXXL's own */
static float vel_curve(int type, int v)
{
    static const float EXP[6] = {2.2f, 1.7f, 1.35f, 1.f, 0.75f, 0.55f};
    const float x = v / 127.f;
    if (type >= 0 && type < 6) return powf(x, EXP[type]);
    if (type == 6) return 0.45f + 0.55f * powf(x, 0.8f) * 0.6f + 0.25f * x;
    if (type == 7) return 0.6f + 0.3f * x;
    return 1.f;   /* Const127 */
}

/* ---------------------------------------------------------------- derived values ---- */
/* P23 / P24 destinations: field (>= 0 timbre, <= -100 common: -(100 + C_x)), -1 none */
static const int MW_DEST[32] = {
    -(100 + C_BALANCE), T_PORTA, T_SPREAD, T_VM_DEPTH, T_V1_PITCH, T_V1_SHAPE, T_V2_PITCH, T_V2_SHAPE, T_XMOD, T_PEG_INT,
    -2 /* multi shape */, T_LV1, T_LV2, T_LVM, T_CUTOFF, T_RESO, T_CUT_EG, T_AA, T_AD, T_AS, T_AR, T_EA, T_ED, T_ES, T_ER,
    T_LFO_RATE, T_LFO_INT, -(100 + C_MFX_SPEED), -(100 + C_MFX_DEPTH), -(100 + C_DR_TIME), -(100 + C_DR_DEPTH), -(100 + C_ARP_GATE)};
const char *const MW_NAMES[32] = {"BALANCE", "PORTAMENTO", "V.SPREAD", "V.M DEPTH", "VCO1 PITCH", "VCO1 SHAPE",
    "VCO2 PITCH", "VCO2 SHAPE", "CROSS MOD", "PITCH EG INT", "MULTI SHAPE", "VCO1 LEVEL", "VCO2 LEVEL", "MULTI LEVEL",
    "CUTOFF", "RESONANCE", "CUTOFF EG INT", "A.EG ATTACK", "A.EG DECAY", "A.EG SUSTAIN", "A.EG RELEASE", "EG ATTACK",
    "EG DECAY", "EG SUSTAIN", "EG RELEASE", "LFO RATE", "LFO INT", "MOD FX SPEED", "MOD FX DEPTH", "DL/RV TIME",
    "DL/RV DEPTH", "GATE TIME"};

/* EG times (OWN taper, KORG gives none): attack 0.5 ms .. ~3.5 s, decay / release 1 ms .. ~17.5 s */
float tsec_att(float x) { const float u = x / 1023.f; return 0.0005f + 3.5f * u * u * u; }
float tsec_dr(float x) { const float u = x / 1023.f; return 0.001f + 17.5f * u * u * u * sqrtf(u); }

static float lfo_hz(int mode, int rate)
{
    /* KORG: SLOW 0.05 - 28 Hz, FAST 0.5 Hz - 2.8 kHz; BPM: P22 divisions (whole notes), here at 120 BPM (the voice
     * scales by the tempo) */
    static const float DIV[16] = {4, 2, 1, 0.75f, 0.5f, 0.375f, 1.f / 3, 0.25f, 0.1875f, 1.f / 6, 0.125f, 1.f / 12,
                                  0.0625f, 1.f / 24, 1.f / 32, 1.f / 36};
    if (mode == 1) return 0.05f * powf(560.f, rate / 1023.f);
    if (mode == 2) return 0.5f * powf(5600.f, rate / 1023.f);
    return 1.f / (DIV[(rate >> 6) & 15] * 4.f * 60.f / 120.f);
}

void synth_derive(synth_t *s, int t)
{
    const int16_t *f = s->prog.t[t];
    float val[T_COUNT];
    tparams_t *p = &s->tp[t];
    int i, mt;
    for (i = 0; i < T_COUNT; i++) val[i] = (float)f[i];
    /* mod wheel and aftertouch (P23 lists) */
    {
        const float w = s->modwheel / 127.f * (float)f[T_MW_RANGE] / 100.f;
        const float a = s->aftertouch / 127.f;
        const int dw = MW_DEST[f[T_MW_ASSIGN] & 31], da = MW_DEST[f[T_AT_ASSIGN] & 31];
        if (dw >= 0) val[dw] += w * (float)(T_FIELDS[dw].max - T_FIELDS[dw].min);
        if (da >= 0) val[da] += a * (float)(T_FIELDS[da].max - T_FIELDS[da].min);
        s->mwShape[t] = (dw == -2 ? w : 0.f) + (da == -2 ? a : 0.f);
        for (i = 0; i < C_COUNT; i++) s->cOff[i] = 0;
        if (dw <= -100) s->cOff[-(dw + 100)] += w * (float)(C_FIELDS[-(dw + 100)].max - C_FIELDS[-(dw + 100)].min);
        if (da <= -100) s->cOff[-(da + 100)] += a * (float)(C_FIELDS[-(da + 100)].max - C_FIELDS[-(da + 100)].min);
    }
    for (i = 0; i < T_COUNT; i++) val[i] = clampf(val[i], T_FIELDS[i].min, T_FIELDS[i].max);
    p->v1wave = f[T_V1_WAVE]; p->v2wave = f[T_V2_WAVE]; p->syncRing = f[T_SYNC_RING]; p->pegTgt = f[T_PEG_TGT];
    p->mType = mt = f[T_M_TYPE]; p->selNoise = f[T_SEL_NOISE] & 3; p->selVpm = f[T_SEL_VPM] & 15; p->selUser = f[T_SEL_USER] & 15;
    p->mRoute = f[T_M_ROUTE]; p->drive = f[T_DRIVE]; p->lowcut = f[T_LOWCUT]; p->keytrk = f[T_KEYTRK];
    p->lfoWave = f[T_LFO_WAVE]; p->lfoMode = f[T_LFO_MODE]; p->lfoTgt = f[T_LFO_TGT]; p->lfoTosc = f[T_LFO_TOSC];
    p->vmType = f[T_VM_TYPE]; p->legato = f[T_LEGATO]; p->lfoKsync = f[T_LFO_KSYNC]; p->lfoVsync = f[T_LFO_VSYNC];
    p->v1semi = (float)(2 - f[T_V1_OCT]) * 12.f; p->v2semi = (float)(2 - f[T_V2_OCT]) * 12.f; p->msemi = (float)(2 - f[T_M_OCT]) * 12.f;
    p->v1cents = pl_pitch_cents((int)val[T_V1_PITCH]); p->v2cents = pl_pitch_cents((int)val[T_V2_PITCH]);
    p->v1shape = val[T_V1_SHAPE] / 1023.f; p->v2shape = val[T_V2_SHAPE] / 1023.f; p->xmod = val[T_XMOD] / 1023.f;
    p->pegCents = pl_peg_cents((int)val[T_PEG_INT]);
    p->lv1 = val[T_LV1] / 1023.f; p->lv2 = val[T_LV2] / 1023.f; p->lvm = val[T_LVM] / 1023.f;
    p->cutoffOct = val[T_CUTOFF] / 1023.f * 10.f; p->reso = val[T_RESO] / 1023.f;
    p->cutEg = pl_cut_eg_pct((int)val[T_CUT_EG]) / 100.f; p->cutVel = val[T_CUT_VEL] / 127.f;
    p->ampVel = s->prog.c[C_AMP_VEL] / 127.f;
    p->aA = tsec_att(val[T_AA]); p->aD = tsec_dr(val[T_AD]); p->aS = val[T_AS] / 1023.f; p->aR = tsec_dr(val[T_AR]);
    p->eA = tsec_att(val[T_EA]); p->eD = tsec_dr(val[T_ED]); p->eS = val[T_ES] / 1023.f; p->eR = tsec_dr(val[T_ER]);
    p->lfoHz = lfo_hz(p->lfoMode, (int)val[T_LFO_RATE]);
    p->lfoInt = val[T_LFO_INT] / 511.f;
    p->mShape = mt == 0 ? f[T_SH_NOISE] / 1023.f : mt == 1 ? f[T_SH_VPM] / 1023.f : f[T_SH_USER] / 1023.f;
    p->mShape = clampf(p->mShape + s->mwShape[t], 0, 1);
    p->mShift = mt == 1 ? f[T_SSH_VPM] / 1023.f : mt == 2 ? f[T_SSH_USER] / 1023.f : 0.f;
    p->porta = val[T_PORTA] < 1 ? 0.f : 0.003f * powf(1300.f, (val[T_PORTA] - 1.f) / 127.f);   /* OFF, then 3 ms .. 4 s (OWN) */
    p->spread = val[T_SPREAD] / 127.f; p->vmDepth = val[T_VM_DEPTH];
    for (i = 0; i < 6; i++) { p->vpm[i] = f[T_VPM_P1 + i] / 100.f; p->usr[i] = f[T_USR_P1 + i]; }
    p->bendUp = f[T_BEND_UP]; p->bendDn = f[T_BEND_DN];
}

/* ---------------------------------------------------------------- voices ---- */
static int timbre_voices(const synth_t *s, int t, int *first)
{
    if (!s->prog.c[C_SUB_ON]) { *first = 0; return t == 0 ? PL_VOICES : 0; }
    *first = t ? PL_VOICES / 2 : 0;
    return PL_VOICES / 2;
}

static voice_t *alloc_voice(synth_t *s, int t)
{
    int first, n = timbre_voices(s, t, &first), i;
    voice_t *best = NULL;
    for (i = first; i < first + n; i++) if (!s->v[i].active) return &s->v[i];
    for (i = first; i < first + n; i++)   /* oldest released voice */
        if (!s->v[i].gate && (!best || s->v[i].age < best->age)) best = &s->v[i];
    if (best) return best;
    for (i = first; i < first + n; i++) if (!best || s->v[i].age < best->age) best = &s->v[i];
    return best;
}

static float spread_pan(const synth_t *s, int t, int k)
{
    return (k & 1 ? 1.f : -1.f) * s->tp[t].spread * (0.5f + 0.5f * (float)((k >> 1) & 3) / 3.f);
}

static void start(synth_t *s, voice_t *v, int t, int key, float vel, float transpose, float detune, float level, int legato)
{
    const tparams_t *p = &s->tp[t];
    const float vg = 1.f - p->ampVel * (1.f - vel);
    voice_note_on(v, p, t, key, vel, transpose, detune, level * vg, spread_pan(s, t, v->idx), legato, ++s->ageCounter);
    v->note = key;
}

/* chord intervals (root position) for CHORD mode */
static const int8_t CHORD[14][4] = {
    {0, 7, -1, -1}, {0, 2, 7, -1}, {0, 3, 7, -1}, {0, 4, 7, -1}, {0, 5, 7, -1}, {0, 3, 7, 10}, {0, 4, 7, 10},
    {0, 5, 7, 10}, {0, 4, 7, 11}, {0, 4, 8, -1}, {0, 3, 6, -1}, {0, 3, 6, 10}, {0, 3, 7, 11}, {0, 4, 6, 11}};

static void release_key(synth_t *s, int t, int key)
{
    int i;
    for (i = 0; i < PL_VOICES; i++) if (s->v[i].active && s->v[i].gate && s->v[i].timbre == t && s->v[i].note == key) voice_note_off(&s->v[i]);
}

/* sound one note on a timbre in its voice mode (gain: layer / x-fade / split level) */
static void timbre_note_on(synth_t *s, int t, int key, float vel, float gain)
{
    const tparams_t *p = &s->tp[t];
    int first, n = timbre_voices(s, t, &first), i;
    if (n <= 0) return;
    switch (p->vmType) {
    case 1: {   /* MONO: voices 2-3 one octave down, then voice 4 two octaves down (KORG); levels from DEPTH */
        const float d = p->vmDepth / 1023.f;
        const float l2 = d < 0.5f ? d * 2.f : 1.f, l4 = d > 0.5f ? (d - 0.5f) * 2.f : 0.f;
        const int legato = s->mono[t].n > 0;
        int k;
        mono_push(&s->mono[t], key);
        for (k = 0; k < 4; k++) {
            voice_t *v = &s->v[first + k];
            const float tr = k == 0 ? 0.f : k < 3 ? -12.f : -24.f;
            const float lv = k == 0 ? 1.f : k < 3 ? l2 : l4;
            if (lv <= 0.f && !v->active) continue;
            start(s, v, t, key, vel, tr, k == 1 ? -4.f : k == 2 ? 4.f : 0.f, gain * lv * (k ? 0.8f : 1.f), legato && v->active);
        }
        break;
    }
    case 2: {   /* UNISON: every voice of the timbre, detune 0 - 50 cent (KORG) */
        const float det = p->vmDepth / 1023.f * 50.f;
        const int legato = s->mono[t].n > 0;
        mono_push(&s->mono[t], key);
        for (i = 0; i < n; i++) {
            const float pos = n > 1 ? (float)i / (float)(n - 1) * 2.f - 1.f : 0.f;
            start(s, &s->v[first + i], t, key, vel, 0.f, pos * det, gain / sqrtf((float)n) * 1.6f, legato);
        }
        break;
    }
    case 3: {   /* CHORD: the chord the DEPTH band selects (KORG P13), root position */
        const int c = pl_chord_index((int)p->vmDepth);
        for (i = 0; i < 4 && CHORD[c][i] >= 0; i++) start(s, alloc_voice(s, t), t, key, vel, (float)CHORD[c][i], 0.f, gain * 0.7f, 0);
        break;
    }
    default:    /* POLY; DEPTH > 255 = DUO: two voices per key, the second's level then detune rise (KORG) */
        if (p->vmDepth < 256) start(s, alloc_voice(s, t), t, key, vel, 0.f, 0.f, gain, 0);
        else {
            const float d = (p->vmDepth - 256.f) / 767.f;
            start(s, alloc_voice(s, t), t, key, vel, 0.f, -d * 12.f, gain, 0);
            start(s, alloc_voice(s, t), t, key, vel, 0.f, d * 12.f, gain * clampf(0.3f + d * 1.4f, 0, 1), 0);
        }
    }
}

static void timbre_note_off(synth_t *s, int t, int key)
{
    const tparams_t *p = &s->tp[t];
    if (p->vmType == 1 || p->vmType == 2) {
        int first, n = timbre_voices(s, t, &first), top, i;
        mono_pop(&s->mono[t], key);
        if (s->mono[t].n > 0) {   /* back to the previous held key, legato */
            top = s->mono[t].keys[s->mono[t].n - 1];
            for (i = first; i < first + n; i++) if (s->v[i].active && s->v[i].timbre == t && s->v[i].gate) {
                s->v[i].target = (float)top; s->v[i].note = top;
                if (p->porta <= 0) s->v[i].pitch = (float)top;
            }
            return;
        }
        for (i = first; i < first + n; i++) if (s->v[i].active && s->v[i].timbre == t) voice_note_off(&s->v[i]);
        return;
    }
    release_key(s, t, key);
}

/* which timbres play a key, and how loud (Layer / X-Fade / Split, MAIN/SUB BALANCE) */
static int key_timbres(const synth_t *s, int key, float g[2])
{
    const float bal = (s->prog.c[C_BALANCE] + s->cOff[C_BALANCE]) / 127.f;
    g[0] = 1.f; g[1] = 0.f;
    if (!s->prog.c[C_SUB_ON]) return 1;
    switch (s->prog.c[C_TIMBRE_TYPE]) {
    case 1: {   /* X-FADE: the sub timbre crosses into the main one up the keyboard (OWN range C1 .. C7) */
        const float x = clampf((key - 24) / 72.f, 0, 1);
        g[0] = x; g[1] = 1.f - x;
        break;
    }
    case 2: {   /* SPLIT at the split point; POSITION 0 = Sub<>Main (sub on the left) */
        const int left = key < s->prog.c[C_SPLIT];
        const int sub = s->prog.c[C_POSITION] == 0 ? left : !left;
        g[0] = sub ? 0.f : 1.f; g[1] = sub ? 1.f : 0.f;
        return 2;
    }
    default: g[0] = 1.f; g[1] = 1.f;
    }
    g[0] *= bal < 0.5f ? 1.f : 2.f * (1.f - bal) + 0.0f;
    g[1] *= bal > 0.5f ? 1.f : 2.f * bal;
    { const float a = g[0], b = g[1]; g[0] = a > 1 ? 1 : a; g[1] = b > 1 ? 1 : b; }
    return 2;
}

static int arp_on_timbre(const synth_t *s, int t)
{
    const int tg = s->prog.c[C_ARP_TGT];
    if (!s->prog.c[C_ARP_MODE]) return 0;
    return tg == 0 || (tg == 1 && t == 0) || (tg == 2 && t == 1);
}

/* a key from MIDI or the arpeggiator */
static void play_key(synth_t *s, int key, float vel, int fromArp)
{
    float g[2];
    int t;
    key_timbres(s, key, g);
    for (t = 0; t < 2; t++) {
        if (g[t] <= 0.f) continue;
        if (!fromArp && arp_on_timbre(s, t)) continue;
        if (fromArp && !arp_on_timbre(s, t)) continue;
        timbre_note_on(s, t, key, vel, g[t]);
    }
}
static void stop_key(synth_t *s, int key, int fromArp)
{
    int t;
    for (t = 0; t < 2; t++) {
        if (!fromArp && arp_on_timbre(s, t)) continue;
        if (fromArp && !arp_on_timbre(s, t)) continue;
        timbre_note_off(s, t, key);
    }
}

void synth_all_off(synth_t *s, int hard)
{
    int i;
    for (i = 0; i < PL_VOICES; i++) { if (hard) voice_kill(&s->v[i]); else if (s->v[i].active) voice_note_off(&s->v[i]); }
    s->mono[0].n = s->mono[1].n = 0;
    s->arp.n = 0; s->arp.latchedUp = 0;
    if (s->arp.sounding >= 0) { stop_key(s, s->arp.sounding, 1); s->arp.sounding = -1; }
    if (s->arp.sounding2 >= 0) { stop_key(s, s->arp.sounding2, 1); s->arp.sounding2 = -1; }
    memset(s->keyDown, 0, sizeof s->keyDown);
}

static int play_note(const synth_t *s, int note)
{
    return note + s->prog.c[C_OCTAVE] * 12 + s->prog.c[C_TRANSPOSE] + s->g.transpose;
}

void synth_note_on(synth_t *s, int note, int vel)
{
    const int key = play_note(s, note);
    const float v = vel_curve(s->g.velCurve, vel);
    if (key < 0 || key > 127) return;
    s->keyDown[note & 127] = (uint8_t)(key + 1);
    arp_key(s, key, 1);
    play_key(s, key, v, 0);
}

void synth_note_off(synth_t *s, int note)
{
    const int k1 = s->keyDown[note & 127];
    int key;
    if (!k1) return;
    key = k1 - 1;
    s->keyDown[note & 127] = 0;
    if (s->damper) { s->sustained[key] = 1; arp_key(s, key, 0); return; }
    arp_key(s, key, 0);
    stop_key(s, key, 0);
}

void synth_damper(synth_t *s, int on)
{
    int k;
    s->damper = on;
    if (on) return;
    for (k = 0; k < 128; k++) if (s->sustained[k]) { s->sustained[k] = 0; stop_key(s, k, 0); }
}

/* ---------------------------------------------------------------- arpeggiator ---- */
void mono_push(monostack_t *m, int key)
{
    int i;
    mono_pop(m, key);
    if (m->n >= 16) { for (i = 1; i < 16; i++) m->keys[i - 1] = m->keys[i]; m->n = 15; }
    m->keys[m->n++] = (int8_t)key;
}
void mono_pop(monostack_t *m, int key)
{
    int i, j;
    for (i = 0; i < m->n; i++) if (m->keys[i] == key) { for (j = i + 1; j < m->n; j++) m->keys[j - 1] = m->keys[j]; m->n--; i--; }
}

void arp_key(synth_t *s, int key, int down)
{
    arp_t *a = &s->arp;
    int i;
    if (!s->prog.c[C_ARP_MODE]) return;
    if (down) {
        if (s->prog.c[C_ARP_MODE] == 2 && a->latchedUp) { a->n = 0; a->latchedUp = 0; }   /* LATCH: a new chord */
        for (i = 0; i < a->n; i++) if (a->held[i] == key) return;
        if (a->n < 32) a->held[a->n++] = (int8_t)key;
        a->physical++;
    } else {
        a->physical = a->physical > 0 ? a->physical - 1 : 0;
        if (s->prog.c[C_ARP_MODE] == 2) { if (!a->physical) a->latchedUp = 1; return; }
        for (i = 0; i < a->n; i++) if (a->held[i] == key) { memmove(&a->held[i], &a->held[i + 1], (size_t)(a->n - i - 1)); a->n--; break; }
        if (!a->n) a->pos = 0;
    }
}

static const float ARP_RATE_BEATS[11] = {1.f / 16, 1.f / 12, 1.f / 8, 1.f / 6, 1.f / 4, 0.375f, 1.f / 3, 0.5f, 0.75f, 2.f / 3, 1.f};

static int arp_next(synth_t *s, int *second)
{
    arp_t *a = &s->arp;
    int sorted[32], n = a->n, i, j, oct = s->prog.c[C_ARP_RANGE] + 1, total, idx, type = s->prog.c[C_ARP_TYPE];
    *second = -1;
    if (!n) return -1;
    for (i = 0; i < n; i++) sorted[i] = a->held[i];
    if (type != 0) for (i = 1; i < n; i++) for (j = i; j > 0 && sorted[j - 1] > sorted[j]; j--) { int t = sorted[j]; sorted[j] = sorted[j - 1]; sorted[j - 1] = t; }
    total = n * oct;
    switch (type) {
    case 2: idx = total - 1 - (a->pos % total); break;                                              /* FALL */
    case 3: { const int cyc = total > 1 ? 2 * total - 2 : 1; const int p = a->pos % cyc; idx = p < total ? p : cyc - p; break; } /* RISE FALL */
    case 4: case 5: idx = (int)(rng_u32(&s->rng) % (uint32_t)total);
        if (type == 5) { const int j2 = (int)(rng_u32(&s->rng) % (uint32_t)total); *second = sorted[j2 % n] + 12 * (j2 / n); }
        break;
    default: idx = a->pos % total;                                                                  /* MANUAL / RISE */
    }
    a->pos++;
    return sorted[idx % n] + 12 * (idx / n);
}

static void arp_step(synth_t *s)
{
    arp_t *a = &s->arp;
    int k, k2;
    if (a->sounding >= 0) { stop_key(s, a->sounding, 1); a->sounding = -1; }
    if (a->sounding2 >= 0) { stop_key(s, a->sounding2, 1); a->sounding2 = -1; }
    k = arp_next(s, &k2);
    if (k < 0 || k > 127) return;
    play_key(s, k, 0.85f, 1);
    a->sounding = k;
    if (k2 >= 0 && k2 <= 127 && k2 != k) { play_key(s, k2, 0.85f, 1); a->sounding2 = k2; }
    a->gateLeft = a->stepFrames * (s->prog.c[C_ARP_GATE] + s->cOff[C_ARP_GATE]) / 72.f;
    if (a->gateLeft < 64) a->gateLeft = 64;
}

static void arp_block(synth_t *s, int frames)
{
    arp_t *a = &s->arp;
    const float beats = ARP_RATE_BEATS[s->prog.c[C_ARP_RATE] % 11];
    const float tempo = synth_tempo(s);
    if (!s->prog.c[C_ARP_MODE]) {
        if (a->sounding >= 0) { stop_key(s, a->sounding, 1); a->sounding = -1; }
        if (a->sounding2 >= 0) { stop_key(s, a->sounding2, 1); a->sounding2 = -1; }
        return;
    }
    a->stepFrames = beats * 60.f / tempo * SR;
    if (a->sounding >= 0 || a->sounding2 >= 0) {
        a->gateLeft -= (float)frames;
        if (a->gateLeft <= 0) {
            if (a->sounding >= 0) { stop_key(s, a->sounding, 1); a->sounding = -1; }
            if (a->sounding2 >= 0) { stop_key(s, a->sounding2, 1); a->sounding2 = -1; }
        }
    }
    if (!a->n) { a->phase = 0; return; }
    if (s->host.playing && s->host.tempo > 1) {   /* on MPC's grid */
        const double step = floor(s->host.ppq / beats);
        if (step != a->lastHostStep) { a->lastHostStep = step; arp_step(s); }
    } else {
        if (a->phase <= 0.f) { arp_step(s); a->phase += a->stepFrames; }
        a->phase -= (float)frames;
    }
}

float synth_tempo(const synth_t *s)
{
    if (s->host.tempo > 1) return (float)s->host.tempo;
    return s->prog.c[C_TEMPO] / 10.f;
}

/* ---------------------------------------------------------------- render ---- */
void synth_render(synth_t *s, float *outL, float *outR, int n)
{
    float busL[2][PL_BLOCK], busR[2][PL_BLOCK];
    float bend[2], shared[2];
    int i, t;
    const float tempo = synth_tempo(s);
    synth_derive(s, 0);
    synth_derive(s, 1);
    arp_block(s, n);
    for (t = 0; t < 2; t++) {
        const tparams_t *p = &s->tp[t];
        memset(busL[t], 0, sizeof busL[t]); memset(busR[t], 0, sizeof busR[t]);
        bend[t] = s->bend >= 0 ? s->bend * p->bendUp : s->bend * p->bendDn;
        /* the shared LFO (LFO VOICE SYNC on) */
        {
            float hz = p->lfoMode == 0 ? p->lfoHz * tempo / 120.f : p->lfoHz;
            s->lfoPh[t] += hz * (float)n / SR;
            s->lfoPh[t] -= floorf(s->lfoPh[t]);
            shared[t] = lfo_wave(p->lfoWave, s->lfoPh[t], NULL, NULL);
        }
    }
    for (i = 0; i < PL_VOICES; i++) {
        voice_t *v = &s->v[i];
        if (!v->active) continue;
        t = v->timbre & 1;
        voice_block(v, &s->tp[t], bend[t], shared[t], tempo, tune_cents(s, v->note) / 100.f, busL[t], busR[t], n);
    }
    for (t = 0; t < 2; t++) for (i = 0; i < n; i++) { busL[t][i] = dcb_tick(&s->dc[t][0], busL[t][i]); busR[t][i] = dcb_tick(&s->dc[t][1], busR[t][i]); }
    /* MOD FX and DELAY/REVERB on the routed timbres (MAIN+SUB / MAIN / SUB) */
    {
        const int mRoute = s->prog.c[C_MFX_ROUTE], dRoute = s->prog.c[C_DR_ROUTE];
        const float lvl = powf(10.f, (s->prog.c[C_LEVEL] - 102) / 20.f) * 0.5f;   /* PROGRAM LEVEL: 102 = 0 dB */
        float aL[PL_BLOCK], aR[PL_BLOCK], bL[PL_BLOCK], bR[PL_BLOCK];
        /* bus a = through the mod fx, bus b = around it */
        for (i = 0; i < n; i++) {
            const int m0 = mRoute != 2, m1 = mRoute != 1;
            aL[i] = (m0 ? busL[0][i] : 0) + (m1 ? busL[1][i] : 0);
            aR[i] = (m0 ? busR[0][i] : 0) + (m1 ? busR[1][i] : 0);
            bL[i] = (m0 ? 0 : busL[0][i]) + (m1 ? 0 : busL[1][i]);
            bR[i] = (m0 ? 0 : busR[0][i]) + (m1 ? 0 : busR[1][i]);
        }
        if (s->prog.c[C_MFX_ON]) {
            const int type = s->prog.c[C_MFX_TYPE] & 3;
            const int sub = type == 0 ? s->prog.c[C_MFX_CHO] : type == 1 ? s->prog.c[C_MFX_ENS] : type == 2 ? s->prog.c[C_MFX_PHA] : s->prog.c[C_MFX_FLA];
            modfx_process(&s->mfx, type, sub, clampf((s->prog.c[C_MFX_SPEED] + s->cOff[C_MFX_SPEED]) / 1023.f, 0, 1),
                          clampf((s->prog.c[C_MFX_DEPTH] + s->cOff[C_MFX_DEPTH]) / 1023.f, 0, 1), aL, aR, n);
        }
        for (i = 0; i < n; i++) { aL[i] += bL[i]; aR[i] += bR[i]; }
        if (s->prog.c[C_DR_ON] && s->prog.c[C_DR_TYPE]) {
            /* routing for the delay/reverb: the timbres it leaves out bypass it (re-split from the buses) */
            float cL[PL_BLOCK], cR[PL_BLOCK];
            const int d0 = dRoute != 2, d1 = dRoute != 1;
            if (d0 && d1) { memcpy(cL, aL, sizeof(float) * (size_t)n); memcpy(cR, aR, sizeof(float) * (size_t)n); for (i = 0; i < n; i++) bL[i] = bR[i] = 0; }
            else {
                const int keep = d0 ? 0 : 1;
                for (i = 0; i < n; i++) {
                    const float tot = fabsf(busL[0][i]) + fabsf(busL[1][i]) + 1e-9f, share = fabsf(busL[keep][i]) / tot;
                    cL[i] = aL[i] * share; cR[i] = aR[i] * share; bL[i] = aL[i] - cL[i]; bR[i] = aR[i] - cR[i];
                }
            }
            {
                const float time = clampf((s->prog.c[C_DR_TIME] + s->cOff[C_DR_TIME]) / 1023.f, 0, 1);
                const float depth = clampf((s->prog.c[C_DR_DEPTH] + s->cOff[C_DR_DEPTH]) / 1023.f, 0, 1);
                const float wet = clampf((s->prog.c[C_DRYWET] - 1) / 1023.f, 0, 1);
                if (s->prog.c[C_DR_TYPE] == 1) delay_process(&s->dly, s->prog.c[C_DLY_TYPE], time, depth, wet, tempo, cL, cR, n);
                else reverb_process(&s->rev, s->prog.c[C_REV_TYPE], time, depth, wet, cL, cR, n);
            }
            for (i = 0; i < n; i++) { aL[i] = cL[i] + bL[i]; aR[i] = cR[i] + bR[i]; }
        }
        for (i = 0; i < n; i++) { outL[i] = aL[i] * lvl; outR[i] = aR[i] * lvl; }
    }
    /* L.F. COMP, then MASTER */
    {
        float gr = 0.f;
        const float m = s->g.master * s->g.master * 1.6f;
        if (s->g.lfcOn) gr = lfcomp_process(&s->lfc, s->g.lfcGain, outL, outR, n);
        else s->lfc.grDb = 0.f;
        for (i = 0; i < n; i++) { outL[i] *= m; outR[i] *= m; }
        /* the VU: rests at 0 VU and swings left by the compression (KORG), VU ballistics ~300 ms */
        {
            const float target = powf(10.f, (-gr - 3.f) / 20.f);
            s->vu += (target - s->vu) * (1.f - expf(-(float)n / (0.3f * SR)));
        }
    }
    s->clock += (uint32_t)n;
}
