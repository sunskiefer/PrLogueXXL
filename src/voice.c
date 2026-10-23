/* PrLogueXXL -- one voice: VCO1, VCO2, MULTI ENGINE, mixer, VCF, AMP EG, EG, LFO.
 *
 * Where KORG documents the behaviour it is followed (docs/PROLOGUE_SPEC.md); the rest is PrLogueXXL's own model,
 * marked OWN here:
 *  - VCOs: band-limited (polyBLEP) saw / square / triangle with a slow analog drift (OWN amount).
 *    SHAPE: SQR = pulse width; SAW = the saw minus a half-period-shifted saw, cutting the even harmonics toward a square
 *    (minilogue behaviour, MusicRadar); TRI = a wavefolder adding 3rd/5th harmonics (minilogue behaviour).
 *    SYNC: VCO2 hard-synced to VCO1; RING: VCO1 x VCO2; CROSS MOD: VCO1 modulates VCO2's frequency.
 *  - MULTI ENGINE: NOISE High/Low (10 Hz - 21 kHz), Peak (bandwidth 110 - 880 Hz), Decim (240 Hz - 48 kHz) per KORG;
 *    VPM: carrier + modulator phase modulation, the 16 types and 6 parameters per KORG's descriptions.
 *  - VCF: two-pole low-pass (KORG: "two-pole low pass filter with enhanced sharpness"), self-oscillating;
 *    DRIVE 0/50/100 %, LOW CUT, KEYTRACK 0/50/100 %; CUTOFF 20 Hz - 20 kHz (OWN curve).
 *  - EGs: linear segments (Sound On Sound), attack up to ~3.5 s, decay/release up to ~17.5 s (user-supplied search). */
#include <math.h>
#include <string.h>
#include "synth.h"

extern void user_osc_cycle(int slot, int voice, int note, float frac, float shape, float shift, float lfo, float *out, int n);
extern void user_osc_noteon(int slot, int voice, int note);
extern void user_osc_noteoff(int slot, int voice);

/* ---------------------------------------------------------------- VPM types ---- */
const vpmdef_t VPM_DEF[16] = {
    /* car set  fb    noise env  att   dec   drive evolve */
    {0, 0, 0.0f, 0.0f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* SIN1 basic modulated sine */
    {0, 0, 0.7f, 0.0f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* SIN2 modulator with self-feedback */
    {0, 2, 0.0f, 0.0f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* SIN3 modulator 3 harmonics up */
    {0, 3, 0.0f, 0.0f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* SIN4 modulator 5 harmonics up (5:1 base) */
    {1, 0, 0.0f, 0.0f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* SAW1 basic modulated saw */
    {3, 0, 0.0f, 0.0f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* SAW2 sine-based pseudo saw */
    {2, 0, 0.0f, 0.0f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* SQU1 basic modulated square */
    {4, 1, 0.0f, 0.0f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* SQU2 sine-based pseudo square (2:1..15:1) */
    {5, 4, 0.8f, 0.0f, 0.0f, 0.002f, 0.6f, 2.5f, 0},  /* FAT1 second sub-harmonic modulator, feedback, driven carrier */
    {5, 5, 0.8f, 0.0f, 0.0f, 0.002f, 0.6f, 2.5f, 0},  /* FAT2 half sub-harmonic modulator, feedback, driven carrier */
    {0, 0, 0.0f, 0.7f, 0.0f, 0.002f, 0.6f, 0.f, 0},   /* AIR1 sine carrier modulated by noise */
    {0, 0, 0.0f, 0.35f, 0.0f, 0.002f, 0.6f, 0.f, 0},  /* AIR2 noise + sine */
    {0, 0, 0.0f, 0.0f, 0.8f, 0.001f, 0.35f, 0.f, 0},  /* DECAY1 modulation decays */
    {0, 6, 0.2f, 0.0f, 1.0f, 0.001f, 0.12f, 0.f, 0},  /* DECAY2 strong decaying modulation, 9:1 base */
    {0, 0, 0.4f, 0.1f, 0.5f, 0.4f, 2.5f, 0.f, 1},     /* CREEP complex evolving */
    {3, 0, 0.9f, 0.05f, 0.6f, 0.25f, 1.6f, 0.5f, 2},  /* THROAT complex evolving */
};

float vpm_ratio(int set, float s)
{
    static const float FULL[17] = {0.25f, 0.5f, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    static const float SUB[8] = {0.25f, 0.5f, 0.75f, 1, 1.5f, 2, 3, 4};
    static const float SUBOFF[8] = {0.75f, 1.25f, 1.75f, 2.25f, 2.75f, 3.25f, 3.75f, 4.25f};
    int i;
    s = clampf(s, 0, 1);
    switch (set) {
    case 1: return 2.f + (float)(int)(s * 13.f + 0.5f);                  /* 2..15 */
    case 2: i = (int)(s * 16.f + 0.5f); return FULL[i] + 3.f;             /* +3 harmonics */
    case 3: i = (int)(s * 16.f + 0.5f); return FULL[i] + 4.f;             /* 5:1 base */
    case 4: return SUB[(int)(s * 7.f + 0.5f)];
    case 5: return SUBOFF[(int)(s * 7.f + 0.5f)];
    case 6: return 9.f + (float)(int)(s * 6.f + 0.5f);                    /* 9..15 */
    default: i = (int)(s * 16.f + 0.5f); return FULL[i];
    }
}

/* ---------------------------------------------------------------- band-limited pieces ---- */
static inline float blep(float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}
static inline float wrap1(float x) { return x - floorf(x); }
static inline float saw_bl(float ph, float dt) { return 2.f * ph - 1.f - blep(ph, dt); }
static inline float sqr_bl(float ph, float dt, float pw)
{
    float y = ph < pw ? 1.f : -1.f;
    y += blep(ph, dt);
    y -= blep(wrap1(ph + 1.f - pw), dt);
    return y;
}
/* triangle: leaky-integrated band-limited square */
static inline float tri_bl(float ph, float dt, float *z)
{
    const float sq = sqr_bl(ph, dt, 0.5f);
    *z = *z * 0.9995f + sq * 4.f * dt;
    return *z;
}
static inline float fold(float x)
{
    /* triangle fold into -1..1 */
    x = (x + 1.f) * 0.25f;
    x = x - floorf(x);
    return x < 0.5f ? 4.f * x - 1.f : 3.f - 4.f * x;
}
static float vco(int wave, float ph, float dt, float shape, float *tz)
{
    switch (wave) {
    case 0: return sqr_bl(ph, dt, 0.5f - 0.46f * shape);                                        /* SQR: width */
    case 1: { const float t = tri_bl(ph, dt, tz); return shape > 0.001f ? fold(t * (1.f + 2.6f * shape)) : t; } /* TRI */
    default: return saw_bl(ph, dt) - shape * saw_bl(wrap1(ph + 0.5f), dt);                      /* SAW */
    }
}

/* ---------------------------------------------------------------- envelopes ---- */
static inline void eg_on(eg_t *e, int legato) { if (!legato || e->st == EG_IDLE || e->st == EG_REL) e->st = EG_ATT; }
static inline void eg_off(eg_t *e) { if (e->st != EG_IDLE) { e->st = EG_REL; e->relFrom = e->v; } }
static inline float eg_step(eg_t *e, float a, float d, float s, float r, float dt)
{
    switch (e->st) {
    case EG_ATT: e->v += dt / (a > 0.0005f ? a : 0.0005f); if (e->v >= 1.f) { e->v = 1.f; e->st = EG_DEC; } break;
    case EG_DEC: e->v -= dt / (d > 0.001f ? d : 0.001f) ; if (e->v <= s) { e->v = s; e->st = EG_SUS; } break;
    case EG_SUS: e->v = s; break;
    case EG_REL: e->v -= dt * (e->relFrom > 0.001f ? e->relFrom : 0.001f) / (r > 0.001f ? r : 0.001f);
        if (e->v <= 0.f) { e->v = 0.f; e->st = EG_IDLE; } break;
    default: e->v = 0.f;
    }
    return e->v;
}

float lfo_wave(int wave, float ph, float *hold, rng_t *r)
{
    (void)hold; (void)r;
    switch (wave) {
    case 0: return ph < 0.5f ? 1.f : -1.f;
    case 1: return ph < 0.5f ? 4.f * ph - 1.f : 3.f - 4.f * ph;
    default: return 1.f - 2.f * ph;   /* saw, falling (as a "SAW" LFO usually reads on KORG panels) */
    }
}

/* ---------------------------------------------------------------- voice ---- */
void voice_note_on(voice_t *v, const tparams_t *tp, int timbre, int note, float vel, float transpose, float detune,
                   float level, float pan, int legato, uint32_t age)
{
    const int wasActive = v->active && v->gate;
    if (!v->active) {
        v->ph1 = v->ph2 = v->phm = v->phmod = 0;
        v->tri1 = v->tri2 = 0; v->fbPrev = 0;
        memset(&v->f, 0, sizeof v->f); memset(&v->nf, 0, sizeof v->nf);
        v->lcz = 0; v->out = 0;
        v->ea.st = v->ef.st = EG_IDLE; v->ea.v = v->ef.v = 0;
        if (!v->rng.s) v->rng.s = 0x1234567u + (uint32_t)v->idx * 7919u + age * 104729u;
        v->ph1 = rng_uf(&v->rng); v->ph2 = rng_uf(&v->rng);
        v->cutJit = rng_f(&v->rng) * 0.03f;
        v->drift = rng_f(&v->rng) * 2.f;
        v->lfoPh = rng_uf(&v->rng);
    }
    v->active = 1; v->gate = 1; v->timbre = timbre; v->note = note; v->vel = vel;
    v->transpose = transpose; v->detune = detune; v->level = level; v->pan = pan; v->age = age;
    v->target = (float)note;
    if (!(wasActive && tp->porta > 0.f) && !(tp->porta > 0.f && legato)) v->pitch = (float)note;
    if (tp->porta <= 0.f) v->pitch = (float)note;
    eg_on(&v->ea, legato && tp->legato);
    eg_on(&v->ef, legato && tp->legato);
    if (!(legato && tp->legato)) v->vpmEnv = 0.f, v->decimPh = 0.f;
    if (tp->lfoKsync && !(legato && tp->legato)) v->lfoPh = 0.f;
    if (tp->mType == 2) user_osc_noteon(tp->selUser, v->idx, note);
}

void voice_note_off(voice_t *v) { v->gate = 0; eg_off(&v->ea); eg_off(&v->ef); }
void voice_kill(voice_t *v) { v->active = 0; v->gate = 0; v->ea.st = v->ef.st = EG_IDLE; v->ea.v = v->ef.v = 0; }

/* the noise engine's filters */
static float noise_tick(voice_t *v, const tparams_t *tp, svfc_t *c, int type, float noteHz, float shape, int *decimRateSet,
                        float *decimInc)
{
    float n = rng_f(&v->rng), lp, bp, hp;
    (void)noteHz; (void)decimRateSet;
    switch (type) {
    case 0: svf_tick(&v->nf, c, n, &lp, &bp, &hp); return hp;
    case 1: svf_tick(&v->nf, c, n, &lp, &bp, &hp); return lp;
    case 2: svf_tick(&v->nf, c, n, &lp, &bp, &hp); return bp * 2.5f;
    default:
        v->decimPh += *decimInc;
        if (v->decimPh >= 1.f) { v->decimPh -= floorf(v->decimPh); v->decimHold = n; }
        (void)tp; (void)shape;
        return v->decimHold;
    }
}

void voice_block(voice_t *v, const tparams_t *tp, float bendSemi, float sharedLfo, float tempo, float tune,
                 float *L, float *R, int n)
{
    const int vi = v->idx;
    float gl, gr;
    int i0;
    if (!v->active) return;
    pan_gains(clampf(v->pan, -1.f, 1.f), &gl, &gr);
    for (i0 = 0; i0 < n; i0 += PL_CTRL) {
        const int m = n - i0 < PL_CTRL ? n - i0 : PL_CTRL;
        const float dtc = (float)m / SR;
        float lfo, ampE, filE, note, n1, n2, nm, f1, f2, fm, dt1, dt2, dtm, sh1, sh2, shm, cutOct, cutHz, res, k;
        float lfoPitch1 = 0, lfoPitch2 = 0, lfoPitchM = 0, lfoSh1 = 0, lfoSh2 = 0, lfoShM = 0, peg, pegA = 0, pegB = 0, pegM = 0;
        float drvG, drvC, mBuf[PL_CTRL], xm, ratio = 1, idx = 0, fb = 0, nz = 0, envI = 0, att, dec, ktF;
        svfc_t fc, nc;
        float decimInc = 0;
        int i, useM = tp->lvm > 0.0005f;
        /* glide */
        if (tp->porta > 0.f) v->pitch += (v->target - v->pitch) * (1.f - expf(-dtc / tp->porta));
        else v->pitch = v->target;
        /* LFO */
        {
            float hz = tp->lfoHz;
            if (tp->lfoMode == 0) hz = tp->lfoHz * (tempo > 1 ? tempo : 120.f) / 120.f;
            if (tp->lfoVsync) lfo = sharedLfo;
            else {
                v->lfoPh += hz * dtc;
                v->lfoPh -= floorf(v->lfoPh);
                lfo = lfo_wave(tp->lfoWave, v->lfoPh, &v->lfoHold, &v->rng);
            }
            lfo *= tp->lfoInt;
        }
        if (tp->lfoTgt == 2) {   /* PITCH: up to +-12 semitones (SLOW/BPM), +-24 (FAST); OWN depths */
            const float semi = lfo * (tp->lfoMode == 2 ? 24.f : 12.f);
            if (tp->lfoTosc == 0 || tp->lfoTosc == 1) lfoPitch1 = semi;
            if (tp->lfoTosc <= 2) lfoPitch2 = semi;
            if (tp->lfoTosc == 0 || tp->lfoTosc == 3) lfoPitchM = semi;
        } else if (tp->lfoTgt == 1) {
            if (tp->lfoTosc == 0 || tp->lfoTosc == 1) lfoSh1 = lfo;
            if (tp->lfoTosc <= 2) lfoSh2 = lfo;
            if (tp->lfoTosc == 0 || tp->lfoTosc == 3) lfoShM = lfo;
        }
        ampE = eg_step(&v->ea, tp->aA, tp->aD, tp->aS, tp->aR, dtc);
        filE = eg_step(&v->ef, tp->eA, tp->eD, tp->eS, tp->eR, dtc);
        if (v->ea.st == EG_IDLE && !v->gate) { voice_kill(v); return; }
        /* pitch EG (KORG: VCO2 / VCO1+2 / ALL incl. multi) */
        peg = tp->pegCents * filE / 100.f;
        if (tp->pegTgt >= 0) pegB = peg;
        if (tp->pegTgt >= 1) pegA = peg;
        if (tp->pegTgt >= 2) pegM = peg;
        /* analog drift: a slow wander of a few cents (OWN) */
        if (rng_uf(&v->rng) < 0.02f) v->driftT = rng_f(&v->rng) * 3.f;
        v->drift += (v->driftT - v->drift) * 0.01f;
        note = v->pitch + v->transpose + bendSemi + tune + (v->detune + v->drift) / 100.f;
        n1 = note + tp->v1semi + tp->v1cents / 100.f + pegA + lfoPitch1;
        n2 = note + tp->v2semi + tp->v2cents / 100.f + pegB + lfoPitch2;
        nm = note + tp->msemi + pegM + lfoPitchM;
        f1 = midi_hz(n1); f2 = midi_hz(n2); fm = midi_hz(nm);
        dt1 = clampf(f1 / SR, 0, 0.45f); dt2 = clampf(f2 / SR, 0, 0.45f); dtm = clampf(fm / SR, 0, 0.45f);
        sh1 = clampf(tp->v1shape + lfoSh1, 0, 1); sh2 = clampf(tp->v2shape + lfoSh2, 0, 1);
        shm = clampf(tp->mShape + lfoShM, 0, 1);
        /* filter (cutoff in octaves above 20 Hz) */
        {
            float egAmt = tp->cutEg * (1.f - tp->cutVel * (1.f - v->vel));
            cutOct = tp->cutoffOct + v->cutJit + egAmt * 8.f * filE + (tp->keytrk ? (v->pitch + v->transpose - 60.f) / 12.f * (tp->keytrk == 1 ? 0.5f : 1.f) : 0)
                     + (tp->lfoTgt == 0 ? lfo * 4.f : 0.f);
            cutHz = 20.f * exp2f(clampf(cutOct, 0, 10.2f));
            res = tp->reso;
            k = 2.f - 2.08f * powf(res, 0.7f);   /* to slightly negative damping: self-oscillation, held by the saturator */
            svf_coef(&fc, cutHz, 1.f);
            fc.k = k;
            fc.a1 = 1.f / (1.f + fc.g * (fc.g + fc.k)); fc.a2 = fc.g * fc.a1; fc.a3 = fc.g * fc.a2;
        }
        drvG = tp->drive == 0 ? 1.f : tp->drive == 1 ? 2.2f : 4.5f;
        drvC = tp->drive == 0 ? 1.f : 1.f / sat(drvG * 0.6f) * 0.6f;
        /* multi engine set-up */
        xm = tp->xmod * tp->xmod * 6.f;
        if (useM) {
            if (tp->mType == 0) {
                const float s = shm;
                if (tp->selNoise == 0) svf_coef(&nc, 10.f * powf(2100.f, s), 0.707f);
                else if (tp->selNoise == 1) svf_coef(&nc, 10.f * powf(2100.f, s), 0.707f);
                else if (tp->selNoise == 2) { const float bw = 110.f * powf(8.f, s); svf_coef(&nc, fm, clampf(fm / bw, 0.5f, 30.f)); }
                else {
                    /* DECIM: rate 240 Hz - 48 kHz; the multi OCTAVE switch sets key tracking (16' none .. 2' full; OWN steps) */
                    static const float KT[4] = {1.f, 0.66f, 0.33f, 0.f};
                    const float kt = KT[(int)clampf(tp->msemi / 12.f + 2.f, 0, 3)];
                    float rate = 240.f * powf(200.f, s) * exp2f((v->pitch - 60.f) / 12.f * kt);
                    decimInc = rate / SR;
                }
            } else if (tp->mType == 1) {
                const vpmdef_t *d = &VPM_DEF[tp->selVpm & 15];
                ratio = vpm_ratio(d->ratioSet, tp->mShift);
                fb = clampf(d->fb + tp->vpm[0], 0, 1.6f);
                nz = clampf(d->noise + tp->vpm[1], 0, 1.f);
                envI = d->envInt * (1.f + tp->vpm[2]);
                if (d->envInt == 0.f && tp->vpm[2] > 0) envI = tp->vpm[2];
                att = d->att * powf(8.f, tp->vpm[3]);
                dec = d->dec * powf(8.f, tp->vpm[4]);
                ktF = exp2f(-tp->vpm[5] * (v->pitch - 60.f) / 24.f);
                /* internal EG: attack to 1, decay to 0 */
                if (v->vpmEnv >= 0.f && v->gate) {
                    if (v->decimPh < 1.f) { v->decimPh += dtc / (att > 0.0005f ? att : 0.0005f); v->vpmEnv = v->decimPh > 1.f ? 1.f : v->decimPh; }
                    else v->vpmEnv *= expf(-dtc / (dec > 0.001f ? dec : 0.001f));
                }
                /* depth: SHAPE = modulator depth 1.00 - 15.00 (KORG), as PM index (OWN scale) */
                idx = (1.f + 14.f * shm) * 0.33f * ktF;
                if (envI > 0) idx *= (1.f - envI) + envI * v->vpmEnv;
                if (d->evolve == 1) ratio *= 1.f + 0.02f * fsin_ph(v->lfoPh * 0.37f + v->age * 0.1f);
                if (d->evolve == 2) idx *= 0.8f + 0.4f * (0.5f + 0.5f * fsin_ph(v->phmod * 0.0001 + v->vpmEnv));
            }
        }
        /* user oscillator renders its own block */
        if (useM && tp->mType == 2) {
            float fr = nm - floorf(nm);
            user_osc_cycle(tp->selUser, vi, (int)floorf(nm), fr, shm, tp->mShift, lfoShM, mBuf, m);
        }
        for (i = 0; i < m; i++) {
            float o1, o2, om = 0, mix, lp, bp, hp, x, ph2old = (float)v->ph2;
            int wrapped = 0;
            /* VCO1 */
            v->ph1 += dt1;
            if (v->ph1 >= 1.0) { v->ph1 -= 1.0; wrapped = 1; }
            o1 = vco(tp->v1wave, (float)v->ph1, dt1, sh1, &v->tri1);
            /* VCO2 (cross mod: VCO1 modulates VCO2's frequency) */
            v->ph2 += dt2 * (1.f + xm * o1);
            if (v->ph2 < 0) v->ph2 += 1.0;
            if (tp->syncRing == 2 && wrapped) v->ph2 = v->ph1 * (dt2 / (dt1 > 1e-6f ? dt1 : 1e-6f));
            if (v->ph2 >= 1.0) v->ph2 -= floor(v->ph2);
            (void)ph2old;
            o2 = vco(tp->v2wave, (float)v->ph2, dt2, sh2, &v->tri2);
            if (tp->syncRing == 0) o2 *= o1;   /* RING: VCO1 x VCO2 */
            /* MULTI */
            if (useM) {
                if (tp->mType == 0) om = noise_tick(v, tp, &nc, tp->selNoise, fm, shm, NULL, &decimInc) * 0.8f;
                else if (tp->mType == 1) {
                    const vpmdef_t *d = &VPM_DEF[tp->selVpm & 15];
                    float mod, pm, car;
                    v->phmod += dtm * ratio;
                    if (v->phmod >= 1.0) v->phmod -= floor(v->phmod);
                    mod = fsin_ph((float)v->phmod + fb * v->fbPrev * 0.159f);
                    v->fbPrev = mod;
                    if (nz > 0) mod += nz * rng_f(&v->rng) * 1.5f;
                    v->phm += dtm;
                    if (v->phm >= 1.0) v->phm -= 1.0;
                    pm = (float)v->phm + idx * mod * 0.159f;
                    pm -= floorf(pm);
                    switch (d->carrier) {
                    case 1: car = saw_bl(pm, dtm); break;
                    case 2: car = sqr_bl(pm, dtm, 0.5f); break;
                    case 3: car = fsin_ph(pm + 0.25f * fsin_ph(pm)); car = fsin_ph(pm + 0.30f * car); break;   /* pseudo saw */
                    case 4: car = fsin_ph(pm) + 0.33f * fsin_ph(3.f * pm) + 0.2f * fsin_ph(5.f * pm); car *= 0.85f; break;
                    default: car = fsin_ph(pm); break;
                    }
                    if (d->drive > 0) car = sat(car * d->drive) / sat(d->drive);
                    om = car;
                } else om = mBuf[i];
            }
            mix = o1 * tp->lv1 + o2 * tp->lv2 + (tp->mRoute == 0 ? om * tp->lvm : 0.f);
            /* LOW CUT: a one-pole high-pass before the filter (OWN corner, 90 Hz) */
            if (tp->lowcut) { v->lcz += 0.0127f * (mix - v->lcz); mix -= v->lcz; }
            x = mix * 0.45f;
            if (tp->drive) x = sat(x * drvG) * drvC;
            svf_tick(&v->f, &fc, x, &lp, &bp, &hp);
            v->f.ic1 = sat(v->f.ic1 * 0.8f) * 1.25f;   /* the analog filter's limiting: bounded self-oscillation */
            lp += tp->mRoute == 1 ? om * tp->lvm * 0.45f : 0.f;
            v->out = lp * ampE * v->level;
            L[i0 + i] += v->out * gl;
            R[i0 + i] += v->out * gr;
        }
    }
}
