/* PrLogueXXL -- the front panel: plugin parameters <-> the program, the PROGRAM/VALUE knob, PROGRAM EDIT and GLOBAL
 * EDIT menus, the OLED lines, the 7-segment display, the button LEDs, programs (bank file) and the plugin state.
 *
 * As on the prologue-16, the panel edits the timbre the TIMBRE "Panel" switch selects (Main, Main+Sub = both, Sub);
 * PROGRAM EDIT pages 1-8 hold the rest of a program (KORG's page layout, docs/PROLOGUE_SPEC.md). */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "synthint.h"

#define SEC(x) ((uint32_t)((x) * SR))

/* ---------------------------------------------------------------- panel <-> program map ---- */
typedef struct { int16_t p; char kind; int16_t f; } map_t;
static const map_t MAP[] = {
    {P_MODFX_SPEED, 'C', C_MFX_SPEED}, {P_MODFX_DEPTH, 'C', C_MFX_DEPTH}, {P_DLRV_TIME, 'C', C_DR_TIME},
    {P_DLRV_DEPTH, 'C', C_DR_DEPTH}, {P_BALANCE, 'C', C_BALANCE}, {P_TIMBRE_TYPE, 'C', C_TIMBRE_TYPE},
    {P_EDIT_TIMBRE, 'C', C_EDIT_TIMBRE}, {P_SUB_ON, 'C', C_SUB_ON}, {P_ARP_MODE, 'C', C_ARP_MODE},
    {P_ARP_RANGE, 'C', C_ARP_RANGE}, {P_ARP_TYPE, 'C', C_ARP_TYPE},
    {P_PORTAMENTO, 'T', T_PORTA}, {P_VOICE_SPREAD, 'T', T_SPREAD}, {P_VM_DEPTH, 'T', T_VM_DEPTH}, {P_VM_TYPE, 'T', T_VM_TYPE},
    {P_VCO1_WAVE, 'T', T_V1_WAVE}, {P_VCO1_OCT, 'T', T_V1_OCT}, {P_VCO1_PITCH, 'T', T_V1_PITCH}, {P_VCO1_SHAPE, 'T', T_V1_SHAPE},
    {P_PEG_TARGET, 'T', T_PEG_TGT}, {P_PEG_INT, 'T', T_PEG_INT},
    {P_VCO2_WAVE, 'T', T_V2_WAVE}, {P_VCO2_OCT, 'T', T_V2_OCT}, {P_VCO2_PITCH, 'T', T_V2_PITCH}, {P_VCO2_SHAPE, 'T', T_V2_SHAPE},
    {P_SYNC_RING, 'T', T_SYNC_RING}, {P_CROSS_MOD, 'T', T_XMOD}, {P_MULTI_TYPE, 'T', T_M_TYPE}, {P_MULTI_OCT, 'T', T_M_OCT},
    {P_VCO1_LEVEL, 'T', T_LV1}, {P_VCO2_LEVEL, 'T', T_LV2}, {P_MULTI_LEVEL, 'T', T_LVM},
    {P_CUTOFF, 'T', T_CUTOFF}, {P_RESONANCE, 'T', T_RESO}, {P_CUTOFF_EG, 'T', T_CUT_EG}, {P_DRIVE, 'T', T_DRIVE},
    {P_LOW_CUT, 'T', T_LOWCUT}, {P_KEYTRACK, 'T', T_KEYTRK},
    {P_AMP_A, 'T', T_AA}, {P_AMP_D, 'T', T_AD}, {P_AMP_S, 'T', T_AS}, {P_AMP_R, 'T', T_AR},
    {P_EG_A, 'T', T_EA}, {P_EG_D, 'T', T_ED}, {P_EG_S, 'T', T_ES}, {P_EG_R, 'T', T_ER},
    {P_LFO_WAVE, 'T', T_LFO_WAVE}, {P_LFO_MODE, 'T', T_LFO_MODE}, {P_LFO_RATE, 'T', T_LFO_RATE}, {P_LFO_INT, 'T', T_LFO_INT},
    {P_LFO_TARGET, 'T', T_LFO_TGT},
};
#define NMAP ((int)(sizeof MAP / sizeof MAP[0]))

static int edit_t(const synth_t *s) { return s->prog.c[C_EDIT_TIMBRE] == 2 ? 1 : 0; }

static void pv_set(synth_t *s, int p, float v)
{
    if (s->pv[p] != v) { s->pv[p] = v; s->rev_++; }
}

static int multi_sel_field(int type) { return type == 0 ? T_SEL_NOISE : type == 1 ? T_SEL_VPM : T_SEL_USER; }
static int multi_shape_field(int type, int shift)
{
    if (type == 0) return T_SH_NOISE;
    if (type == 1) return shift ? T_SSH_VPM : T_SH_VPM;
    return shift ? T_SSH_USER : T_SH_USER;
}

static void sync_multi(synth_t *s)
{
    const int16_t *t = s->prog.t[edit_t(s)];
    const int mt = t[T_M_TYPE];
    pv_set(s, P_MULTI_SEL, t[multi_sel_field(mt)]);
    pv_set(s, P_MULTI_SHAPE, t[multi_shape_field(mt, s->ui.shift)]);
    pv_set(s, P_ME_DISP, (float)(mt == 0 ? (t[T_SEL_NOISE] & 3) : mt == 1 ? 4 + (t[T_SEL_VPM] & 15) : 20 + (t[T_SEL_USER] & 15)));
}

void panel_sync(synth_t *s)
{
    const int et = edit_t(s);
    int i;
    for (i = 0; i < NMAP; i++)
        pv_set(s, MAP[i].p, MAP[i].kind == 'C' ? s->prog.c[MAP[i].f] : s->prog.t[et][MAP[i].f]);
    pv_set(s, P_MODFX_SW, s->prog.c[C_MFX_ON] ? 1 : 0);
    pv_set(s, P_DLRV_SW, s->prog.c[C_DR_ON] ? s->prog.c[C_DR_TYPE] : 0);
    pv_set(s, P_TEMPO, s->prog.c[C_TEMPO] / 10.f);
    pv_set(s, P_MASTER, s->g.master);
    pv_set(s, P_LFC_SW, (float)s->g.lfcOn);
    pv_set(s, P_LFC_GAIN, s->g.lfcGain);
    pv_set(s, P_SHIFT, (float)s->ui.shift);
    pv_set(s, P_PROGRAM, (float)s->progNum);
    sync_multi(s);
    s->rev_++;
}

static void show(synth_t *s, const char *a, const char *b)
{
    snprintf(s->ui.msg1, sizeof s->ui.msg1, "%s", a);
    snprintf(s->ui.msg2, sizeof s->ui.msg2, "%s", b);
    s->ui.msgUntil = s->clock + SEC(2.0);
    s->rev_++;
}

/* ---------------------------------------------------------------- value texts ---- */
static void time_text(float sec, char *b, int n)
{
    if (sec < 1.f) snprintf(b, (size_t)n, "%.0f ms", sec * 1000.f); else snprintf(b, (size_t)n, "%.2f s", sec);
}

static const char *const MFX_NAMES[4] = {"CHORUS", "ENSEMBLE", "PHASER", "FLANGER"};
static const char *const CHO_SUB[8] = {"STEREO", "LIGHT", "DEEP", "TRIPHASE", "HARMONIC", "MONO", "FEEDBACK", "VIBRATO"};
static const char *const ENS_SUB[3] = {"STEREO", "LIGHT", "MONO"};
static const char *const PHA_SUB[8] = {"STEREO", "FAST", "ORANGE", "SMALL", "SMALL RESO", "BLACK", "FORMANT", "TWINKLE"};
static const char *const FLA_SUB[8] = {"STEREO", "LIGHT", "MONO", "HIGH SWEEP", "MID SWEEP", "PAN SWEEP", "MONO SWEEP", "TRIPHASE"};
static const char *const REV_SUB[10] = {"HALL", "SMOOTH", "ARENA", "PLATE", "ROOM", "EARLY REF", "SPACE", "RISER", "SUBMARINE", "HORROR"};
static const char *const DLY_SUB[12] = {"STEREO", "MONO", "PING PONG", "HIPASS", "TAPE", "ONE TAP", "STEREO BPM", "MONO BPM",
                                        "PING BPM", "HIPASS BPM", "TAPE BPM", "DOUBLING"};
static const char *const CATS[9] = {"POLY SYNTH", "BASS", "LEAD", "PAD/STRINGS", "KEY/BELL", "CHORD", "ARP", "COMBINATION", "SFX"};
static const char *const VM_NAMES[4] = {"POLY", "MONO", "UNISON", "CHORD"};
static const char *const ARP_RATE_NAMES[11] = {"64th", "48th", "32th", "24th", "16th", "16.t", "12th", "8th", "8.th", "6th", "4th"};
static const char *const LFO_DIV[16] = {"4", "2", "1", "3/4", "1/2", "3/8", "1/3", "1/4", "3/16", "1/6", "1/8", "1/12",
                                        "1/16", "1/24", "1/32", "1/36"};
/* the waves user oscillator's parameters (its manifest.json) */
static const char *const USER_PNAME[6] = {"Wave A", "Wave B", "Sub Wave", "Sub Mix", "Ring Mix", "Bit Crush"};
static const int USER_PMAX[6] = {45, 43, 15, 100, 100, 100};

static const char *mfx_sub_name(const program_t *p)
{
    switch (p->c[C_MFX_TYPE] & 3) {
    case 0: return CHO_SUB[p->c[C_MFX_CHO] & 7];
    case 1: return ENS_SUB[p->c[C_MFX_ENS] % 3];
    case 2: return PHA_SUB[p->c[C_MFX_PHA] & 7];
    default: return FLA_SUB[p->c[C_MFX_FLA] & 7];
    }
}

static void tfield_text(const synth_t *s, int t, int f, int v, char *b, int n)
{
    const int16_t *tf = s->prog.t[t];
    switch (f) {
    case T_V1_PITCH: case T_V2_PITCH: snprintf(b, (size_t)n, "%+.0f cent", pl_pitch_cents(v)); return;
    case T_PEG_INT: snprintf(b, (size_t)n, "%+.0f cent", pl_peg_cents(v)); return;
    case T_CUT_EG: snprintf(b, (size_t)n, "%+.1f %%", pl_cut_eg_pct(v)); return;
    case T_AA: case T_EA: time_text(tsec_att((float)v), b, n); return;
    case T_AD: case T_AR: case T_ED: case T_ER: time_text(tsec_dr((float)v), b, n); return;
    case T_PORTA: if (v <= 0) snprintf(b, (size_t)n, "OFF"); else snprintf(b, (size_t)n, "%d", v - 1); return;
    case T_LFO_RATE:
        if (tf[T_LFO_MODE] == 0) snprintf(b, (size_t)n, "%s", LFO_DIV[(v >> 6) & 15]);
        else { const float hz = tf[T_LFO_MODE] == 1 ? 0.05f * powf(560.f, v / 1023.f) : 0.5f * powf(5600.f, v / 1023.f);
               snprintf(b, (size_t)n, hz < 10 ? "%.2f Hz" : hz < 100 ? "%.1f Hz" : "%.0f Hz", hz); }
        return;
    case T_VM_DEPTH:
        switch (tf[T_VM_TYPE]) {
        case 0: if (v < 256) snprintf(b, (size_t)n, "Poly"); else snprintf(b, (size_t)n, "Duo %d", (v - 256) * 1023 / 767); return;
        case 1: snprintf(b, (size_t)n, "Sub %d", v); return;
        case 2: snprintf(b, (size_t)n, "Detune %.1f C", v * 50.f / 1023.f); return;
        default: snprintf(b, (size_t)n, "%s", CHORD_NAMES[pl_chord_index(v)]); return;
        }
    case T_VPM_P1: case T_VPM_P2: case T_VPM_P3: case T_VPM_P4: case T_VPM_P5: case T_VPM_P6: case T_MW_RANGE:
        snprintf(b, (size_t)n, "%+d %%", v); return;
    case T_MW_ASSIGN: case T_AT_ASSIGN: snprintf(b, (size_t)n, "%s", MW_NAMES[v & 31]); return;
    case T_BEND_UP: case T_BEND_DN: if (!v) snprintf(b, (size_t)n, "OFF"); else snprintf(b, (size_t)n, "%c%d Note", f == T_BEND_UP ? '+' : '-', v); return;
    case T_LFO_TOSC: { static const char *const N[4] = {"All", "VCO1+2", "VCO2", "Multi"}; snprintf(b, (size_t)n, "%s", N[v & 3]); return; }
    case T_LFO_KSYNC: case T_LFO_VSYNC: case T_LEGATO: case T_LOWCUT: snprintf(b, (size_t)n, v ? "On" : "Off"); return;
    case T_M_ROUTE: snprintf(b, (size_t)n, v ? "Post VCF" : "Pre VCF"); return;
    default: snprintf(b, (size_t)n, "%d", v);
    }
}

static void cfield_text(const synth_t *s, int f, int v, char *b, int n)
{
    switch (f) {
    case C_TEMPO: snprintf(b, (size_t)n, "%.1f BPM", v / 10.f); return;
    case C_LEVEL: snprintf(b, (size_t)n, "%+d dB", v - 102 + 0); return;
    case C_SPLIT: { static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        snprintf(b, (size_t)n, "%s%d", NN[v % 12], v / 12 - 1); return; }
    case C_SUB_ON: case C_MFX_ON: case C_DR_ON: snprintf(b, (size_t)n, v ? "On" : "Off"); return;
    case C_EDIT_TIMBRE: { static const char *const N[3] = {"Main", "Main+Sub", "Sub"}; snprintf(b, (size_t)n, "%s", N[v % 3]); return; }
    case C_TIMBRE_TYPE: { static const char *const N[3] = {"Layer", "XFade", "Split"}; snprintf(b, (size_t)n, "%s", N[v % 3]); return; }
    case C_POSITION: snprintf(b, (size_t)n, v ? "Main<>Sub" : "Sub<>Main"); return;
    case C_ARP_TGT: case C_MFX_ROUTE: case C_DR_ROUTE: { static const char *const N[3] = {"Main+Sub", "Main", "Sub"}; snprintf(b, (size_t)n, "%s", N[v % 3]); return; }
    case C_PORTA_MODE: snprintf(b, (size_t)n, v ? "On" : "Auto"); return;
    case C_MFX_TYPE: snprintf(b, (size_t)n, "%s", MFX_NAMES[v & 3]); return;
    case C_DR_TYPE: { static const char *const N[3] = {"Off", "Delay", "Reverb"}; snprintf(b, (size_t)n, "%s", N[v % 3]); return; }
    case C_REV_TYPE: snprintf(b, (size_t)n, "%s", REV_SUB[v % 10]); return;
    case C_DLY_TYPE: snprintf(b, (size_t)n, "%s", DLY_SUB[v % 12]); return;
    case C_TUNING: snprintf(b, (size_t)n, "%s", TUNING_NAMES[v % 6]); return;
    case C_ARP_RATE: snprintf(b, (size_t)n, "%s", ARP_RATE_NAMES[v % 11]); return;
    case C_ARP_GATE: snprintf(b, (size_t)n, "%d %%", v * 100 / 72); return;
    case C_ARP_MODE: { static const char *const N[3] = {"Off", "On", "Latch"}; snprintf(b, (size_t)n, "%s", N[v % 3]); return; }
    case C_ARP_RANGE: snprintf(b, (size_t)n, "%d Oct", v + 1); return;
    case C_ARP_TYPE: { static const char *const N[6] = {"Manual", "Rise", "Fall", "Rise Fall", "Random", "Poly Random"}; snprintf(b, (size_t)n, "%s", N[v % 6]); return; }
    case C_CATEGORY: snprintf(b, (size_t)n, "%s", CATS[v % 9]); return;
    case C_DRYWET: snprintf(b, (size_t)n, "%d %%", (v - 1) * 100 / 1023); return;
    case C_PRG_TUNE: snprintf(b, (size_t)n, "%+d cent", v); return;
    case C_SCALE_KEY: case C_TRANSPOSE: snprintf(b, (size_t)n, "%+d Note", v); return;
    case C_OCTAVE: snprintf(b, (size_t)n, "%+d", v); return;
    default: (void)s; snprintf(b, (size_t)n, "%d", v);
    }
}

int panel_text(synth_t *s, int p, char *b, int n)
{
    const int et = edit_t(s);
    int i;
    for (i = 0; i < NMAP; i++) if (MAP[i].p == p) {
        if (MAP[i].kind == 'C') cfield_text(s, MAP[i].f, s->prog.c[MAP[i].f], b, n);
        else tfield_text(s, et, MAP[i].f, s->prog.t[et][MAP[i].f], b, n);
        return 1;
    }
    switch (p) {
    case P_MULTI_SEL: {
        const int mt = s->prog.t[et][T_M_TYPE];
        const int v = s->prog.t[et][multi_sel_field(mt)];
        snprintf(b, (size_t)n, "%s", mt == 0 ? NOISE_NAMES[v & 3] : mt == 1 ? VPM_NAMES[v & 15] : USER_NAMES[v & 15]);
        return 1;
    }
    case P_MULTI_SHAPE: {
        const int mt = s->prog.t[et][T_M_TYPE], v = s->prog.t[et][multi_shape_field(mt, s->ui.shift)];
        if (mt == 0) {
            const int nt = s->prog.t[et][T_SEL_NOISE] & 3;
            const float x = v / 1023.f;
            if (nt <= 1) snprintf(b, (size_t)n, "%.0f Hz", 10.f * powf(2100.f, x));
            else if (nt == 2) snprintf(b, (size_t)n, "BW %.0f Hz", 110.f * powf(8.f, x));
            else snprintf(b, (size_t)n, "%.0f Hz", 240.f * powf(200.f, x));
        } else if (mt == 1 && !s->ui.shift) snprintf(b, (size_t)n, "Depth %.2f", 1.f + 14.f * v / 1023.f);
        else if (mt == 1) {
            const float r = vpm_ratio(VPM_DEF[s->prog.t[et][T_SEL_VPM] & 15].ratioSet, v / 1023.f);
            if (r < 1.f) snprintf(b, (size_t)n, "Ratio 1:%.0f", 1.f / r); else snprintf(b, (size_t)n, "Ratio %g:1", r);
        } else snprintf(b, (size_t)n, "%d", v);
        return 1;
    }
    case P_MASTER: snprintf(b, (size_t)n, "%.0f %%", s->g.master * 100.f); return 1;
    case P_LFC_GAIN: snprintf(b, (size_t)n, "%.0f", s->g.lfcGain * 100.f); return 1;
    case P_BEND: snprintf(b, (size_t)n, "%+.2f", s->bend); return 1;
    case P_PROGRAM: program_name(s, (int)s->pv[P_PROGRAM], b, n); return 1;
    case P_VALUE: snprintf(b, (size_t)n, " "); return 1;
    default: return 0;
    }
}

/* ---------------------------------------------------------------- menus ---- */
typedef struct { const char *name; char kind; int16_t f; } item_t;   /* kind: C common, T timbre, N name char, S subtype */
static const char *const PAGE_NAMES[8] = {"SORT", "NAME", "TIMBRE", "PEDAL & WHEEL", "MODULATION", "OTHER", "MULTI ENGINE", "EFFECTS"};
static const item_t PG_SORT[] = {{"Category", 'C', C_CATEGORY}};
static const item_t PG_TIMBRE[] = {{"Sub Timbre", 'C', C_SUB_ON}, {"Edit Timbre", 'C', C_EDIT_TIMBRE}, {"Timbre Type", 'C', C_TIMBRE_TYPE},
    {"Main/Sub Balance", 'C', C_BALANCE}, {"Main/Sub Position", 'C', C_POSITION}, {"Split Point", 'C', C_SPLIT}};
static const item_t PG_WHEEL[] = {{"M.Wheel Assign", 'T', T_MW_ASSIGN}, {"M.Wheel Range", 'T', T_MW_RANGE},
    {"Bend Range +", 'T', T_BEND_UP}, {"Bend Range -", 'T', T_BEND_DN}, {"AfterTouch", 'T', T_AT_ASSIGN}};
static const item_t PG_MOD[] = {{"LFO Target OSC", 'T', T_LFO_TOSC}, {"LFO Key Sync", 'T', T_LFO_KSYNC},
    {"LFO Voice Sync", 'T', T_LFO_VSYNC}, {"EG Velocity", 'T', T_CUT_VEL}, {"Amp Velocity", 'C', C_AMP_VEL}};
static const item_t PG_OTHER[] = {{"Arp Target", 'C', C_ARP_TGT}, {"Arp Rate", 'C', C_ARP_RATE}, {"Arp Gate Time", 'C', C_ARP_GATE},
    {"Multi Routing", 'T', T_M_ROUTE}, {"EG Legato", 'T', T_LEGATO}, {"Portamento Mode", 'C', C_PORTA_MODE},
    {"Voice Spread", 'T', T_SPREAD}, {"Program Level", 'C', C_LEVEL}, {"Program Tuning", 'C', C_PRG_TUNE},
    {"Transpose", 'C', C_TRANSPOSE}, {"Micro Tuning", 'C', C_TUNING}, {"Scale Key", 'C', C_SCALE_KEY}, {"Tempo", 'C', C_TEMPO}};
static const item_t PG_FX[] = {{"Mod FX Type", 'C', C_MFX_TYPE}, {"Mod FX Subtype", 'S', 0}, {"Mod FX Routing", 'C', C_MFX_ROUTE},
    {"DL/RV Type", 'C', C_DR_TYPE}, {"DL/RV Subtype", 'S', 1}, {"DL/RV Dry Wet", 'C', C_DRYWET}, {"DL/RV Routing", 'C', C_DR_ROUTE}};

static int page_items(const synth_t *s, int page, const item_t **out, item_t *tmp)
{
    int i;
    switch (page) {
    case 0: *out = PG_SORT; return 1;
    case 1: for (i = 0; i < PL_NAME; i++) { tmp[i].name = "Name"; tmp[i].kind = 'N'; tmp[i].f = (int16_t)i; } *out = tmp; return PL_NAME;
    case 2: *out = PG_TIMBRE; return 6;
    case 3: *out = PG_WHEEL; return 5;
    case 4: *out = PG_MOD; return 5;
    case 5: *out = PG_OTHER; return 13;
    case 6: {
        const int mt = s->prog.t[edit_t(s)][T_M_TYPE];
        if (mt == 0) return 0;
        for (i = 0; i < 6; i++) { tmp[i].name = mt == 1 ? T_FIELDS[T_VPM_P1 + i].name : USER_PNAME[i]; tmp[i].kind = 'T';
                                  tmp[i].f = (int16_t)((mt == 1 ? T_VPM_P1 : T_USR_P1) + i); }
        *out = tmp; return 6;
    }
    default: *out = PG_FX; return 7;
    }
}

static const char NAMECH[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+./&!#";

static void item_text(synth_t *s, const item_t *it, char *b, int n)
{
    const int et = edit_t(s);
    if (it->kind == 'C') cfield_text(s, it->f, s->prog.c[it->f], b, n);
    else if (it->kind == 'T') {
        if (it->f >= T_USR_P1 && it->f <= T_USR_P6) snprintf(b, (size_t)n, "%d", s->prog.t[et][it->f]);
        else tfield_text(s, et, it->f, s->prog.t[et][it->f], b, n);
    } else if (it->kind == 'N') snprintf(b, (size_t)n, "%s  [%c]", s->prog.name, s->prog.name[it->f] ? s->prog.name[it->f] : ' ');
    else if (it->f == 0) snprintf(b, (size_t)n, "%s", mfx_sub_name(&s->prog));
    else snprintf(b, (size_t)n, "%s", s->prog.c[C_DR_TYPE] == 2 ? REV_SUB[s->prog.c[C_REV_TYPE] % 10] : DLY_SUB[s->prog.c[C_DLY_TYPE] % 12]);
}

static void user_params_apply(synth_t *s)
{
    int i, t = 0;
    if (s->prog.t[1][T_M_TYPE] == 2 && s->prog.t[0][T_M_TYPE] != 2) t = 1;
    for (i = 0; i < 6; i++) {
        int v = s->prog.t[t][T_USR_P1 + i];
        if (v > USER_PMAX[i]) v = USER_PMAX[i];
        user_osc_param_all(0, i, v);
    }
}

static void tset(synth_t *s, int f, int v)
{
    const int e = s->prog.c[C_EDIT_TIMBRE];
    if (v < T_FIELDS[f].min) v = T_FIELDS[f].min;
    if (v > T_FIELDS[f].max) v = T_FIELDS[f].max;
    if (e != 2) s->prog.t[0][f] = (int16_t)v;
    if (e != 0) s->prog.t[1][f] = (int16_t)v;
    if (f >= T_USR_P1 && f <= T_USR_P6) user_params_apply(s);
    s->ui.dirty = 1;
}
static void cset(synth_t *s, int f, int v)
{
    if (v < C_FIELDS[f].min) v = C_FIELDS[f].min;
    if (v > C_FIELDS[f].max) v = C_FIELDS[f].max;
    s->prog.c[f] = (int16_t)v;
    s->ui.dirty = 1;
}

static int step_size(int lo, int hi) { const int r = hi - lo; return r > 500 ? 8 : r > 200 ? 2 : 1; }

static void item_step(synth_t *s, const item_t *it, int d)
{
    const int et = edit_t(s);
    if (it->kind == 'C') {
        const field_t *f = &C_FIELDS[it->f];
        int st = it->f == C_TEMPO ? 10 : step_size(f->min, f->max);
        cset(s, it->f, s->prog.c[it->f] + d * st);
        if (it->f == C_EDIT_TIMBRE || it->f == C_SUB_ON || it->f == C_TIMBRE_TYPE || it->f == C_BALANCE || it->f == C_TEMPO) panel_sync(s);
        if (it->f == C_SUB_ON) synth_all_off(s, 0);
    } else if (it->kind == 'T') {
        const field_t *f = &T_FIELDS[it->f];
        int hi = f->max;
        if (it->f >= T_USR_P1 && it->f <= T_USR_P6) hi = USER_PMAX[it->f - T_USR_P1];
        {
            int v = s->prog.t[et][it->f] + d * step_size(f->min, f->max);
            if (v > hi) v = hi;
            tset(s, it->f, v);
        }
        panel_sync(s);
    } else if (it->kind == 'N') {
        char *c = &s->prog.name[it->f];
        const char *pos = strchr(NAMECH, *c ? *c : ' ');
        int k = pos ? (int)(pos - NAMECH) : 0;
        const int nc = (int)sizeof NAMECH - 1;
        k = ((k + d) % nc + nc) % nc;
        *c = NAMECH[k];
        { int i; for (i = 0; i < PL_NAME; i++) if (!s->prog.name[i]) s->prog.name[i] = ' '; s->prog.name[PL_NAME] = 0; }
        s->ui.dirty = 1;
    } else if (it->f == 0) {   /* mod fx subtype */
        static const int CF[4] = {C_MFX_CHO, C_MFX_ENS, C_MFX_PHA, C_MFX_FLA};
        const int f = CF[s->prog.c[C_MFX_TYPE] & 3];
        cset(s, f, s->prog.c[f] + d);
    } else {
        const int f = s->prog.c[C_DR_TYPE] == 2 ? C_REV_TYPE : C_DLY_TYPE;
        cset(s, f, s->prog.c[f] + d);
    }
}

/* GLOBAL EDIT (SHIFT + EDIT MODE) */
static const char *const GPAGE_NAMES[2] = {"GLOBAL 1 TUNE", "GLOBAL 2 KEYBOARD"};
static int gl_items(int page) { return page == 0 ? 2 : 1; }
static void gl_text(synth_t *s, int page, int item, char *name, char *b, int n)
{
    static const char *const VC[9] = {"Type 1", "Type 2", "Type 3", "Type 4", "Type 5", "Type 6", "Type 7", "Type 8", "Const127"};
    if (page == 0 && item == 0) { strcpy(name, "Master Tune"); snprintf(b, (size_t)n, "%+d cent", s->g.masterTune); }
    else if (page == 0) { strcpy(name, "Transpose"); snprintf(b, (size_t)n, "%+d Note", s->g.transpose); }
    else { strcpy(name, "Velocity Curve"); snprintf(b, (size_t)n, "%s", VC[s->g.velCurve % 9]); }
}
static void gl_step(synth_t *s, int page, int item, int d)
{
    if (page == 0 && item == 0) s->g.masterTune = (int)clampf((float)(s->g.masterTune + d), -50, 50);
    else if (page == 0) s->g.transpose = (int)clampf((float)(s->g.transpose + d), -12, 12);
    else s->g.velCurve = (int)clampf((float)(s->g.velCurve + d), 0, 8);
}

/* ---------------------------------------------------------------- programs ---- */
void program_name(synth_t *s, int n, char *buf, int len)
{
    if (n < 0) n = 0;
    if (n >= PL_PROGRAMS) n = PL_PROGRAMS - 1;
    snprintf(buf, (size_t)len, "%03d %s", n + 1, s->bank[n].name);
}

void load_program(synth_t *s, int n)
{
    if (n < 0) n = 0;
    if (n >= PL_PROGRAMS) n = PL_PROGRAMS - 1;
    synth_all_off(s, 0);
    s->prog = s->bank[n];
    s->progNum = n;
    s->ui.dirty = 0;
    user_params_apply(s);
    panel_sync(s);
}

static void fav_or_page(synth_t *s, int b)
{
    char t[40];
    if (s->ui.edit) {
        if (s->ui.page == b) s->ui.item++;
        else { s->ui.page = b; s->ui.item = 0; }
        s->rev_++;
        return;
    }
    if (s->ui.global) {
        if (b < 2) { if (s->ui.page == b) s->ui.item = (s->ui.item + 1) % gl_items(b); else { s->ui.page = b; s->ui.item = 0; } }
        s->rev_++;
        return;
    }
    /* outside the menus: 8 favourite programs (SHIFT + button stores the current program) */
    if (s->ui.shift) {
        s->ui.fav[b] = s->progNum;
        snprintf(t, sizeof t, "Favourite %d", b + 1);
        program_name(s, s->progNum, s->ui.msg2, sizeof s->ui.msg2);
        show(s, t, s->ui.msg2);
        s->ui.shift = 0; pv_set(s, P_SHIFT, 0);
    } else if (s->ui.fav[b] >= 0) load_program(s, s->ui.fav[b]);
    else { snprintf(t, sizeof t, "Favourite %d empty", b + 1); show(s, t, "SHIFT + button stores"); }
}

/* the PROGRAM/VALUE knob: an endless encoder (its parameter springs back to the middle when it rests) */
static void value_steps(synth_t *s, int d)
{
    const item_t *items;
    item_t tmp[PL_NAME];
    if (s->ui.edit) {
        const int n = page_items(s, s->ui.page, &items, tmp);
        if (n > 0) item_step(s, &items[s->ui.item % n], d);
    } else if (s->ui.global) gl_step(s, s->ui.page, s->ui.item % gl_items(s->ui.page), d);
    else if (s->ui.ctx == CTX_MFX) {
        static const int CF[4] = {C_MFX_CHO, C_MFX_ENS, C_MFX_PHA, C_MFX_FLA};
        cset(s, CF[s->prog.c[C_MFX_TYPE] & 3], s->prog.c[CF[s->prog.c[C_MFX_TYPE] & 3]] + d);
        s->ui.ctxUntil = s->clock + SEC(3);
    } else if (s->ui.ctx == CTX_DLRV) {
        const int f = s->prog.c[C_DR_TYPE] == 2 ? C_REV_TYPE : C_DLY_TYPE;
        cset(s, f, s->prog.c[f] + d);
        s->ui.ctxUntil = s->clock + SEC(3);
    } else if (s->ui.ctx == CTX_TEMPO) {
        cset(s, C_TEMPO, s->prog.c[C_TEMPO] + d * 10);
        pv_set(s, P_TEMPO, s->prog.c[C_TEMPO] / 10.f);
        s->ui.ctxUntil = s->clock + SEC(3);
    } else {
        int n = s->progNum + d;
        while (n < 0) n += PL_PROGRAMS;
        load_program(s, n % PL_PROGRAMS);
    }
    s->rev_++;
}

/* Is this host write of a momentary button a tap? (device, Force, 2026-10-23)
 * MPC's button is a Toggle Switch fired on touch-down, and a lit button (its "<key>_on" answer, panel_led) holds the
 * host's switch at 1. So a tap on a lit button arrives as 0, not 1: ARP ON/LATCH could never be turned off, WRITE
 * never wrote while the program was dirty, EDIT never left the menu. A 0 is a tap when the button is lit and no touch
 * is still settling: a 1-tap is held at 1 by the wrapper for hold_ms (200 ms) and echoes as a 0 when it lets go, so
 * a 0 within 400 ms of a 1-tap is that echo. Any write within 150 ms of the last tap is the same touch re-firing
 * (a held or wobbling finger ran VOICE MODE SELECT through the modes), not a new tap. */
static int button_press(synth_t *s, int p, int on)
{
    const uint32_t since = s->clock - s->ui.pressAt[p];
    if (since < SEC(0.15)) return 0;
    if (!on && (!panel_led(s, p) || (s->ui.pressOn[p] && since < SEC(0.4)))) return 0;
    s->ui.pressAt[p] = s->clock;
    s->ui.pressOn[p] = (uint8_t)on;
    return 1;
}

/* ---------------------------------------------------------------- a parameter moved ---- */
void panel_set(synth_t *s, int p, float v)
{
    const int on = v >= 0.5f;
    const int et = edit_t(s);
    char t[48];
    int i;
    if (p < 0 || p >= P_COUNT) return;
    /* plain program fields */
    for (i = 0; i < NMAP; i++) if (MAP[i].p == p) {
        const int iv = (int)lrintf(v);
        s->pv[p] = v;
        if (MAP[i].kind == 'C') {
            if (s->prog.c[MAP[i].f] == iv) return;
            cset(s, MAP[i].f, iv);
            if (MAP[i].f == C_EDIT_TIMBRE || MAP[i].f == C_SUB_ON) { if (MAP[i].f == C_SUB_ON) synth_all_off(s, 0); panel_sync(s); }
            if (MAP[i].f == C_ARP_MODE && !iv) { s->arp.n = 0; }
        } else {
            if (s->prog.t[et][MAP[i].f] == iv && s->prog.c[C_EDIT_TIMBRE] != 1) return;
            tset(s, MAP[i].f, iv);
            if (MAP[i].f == T_M_TYPE) sync_multi(s);
            if (MAP[i].f == T_VM_TYPE) synth_all_off(s, 0);
        }
        panel_text(s, p, t, sizeof t);
        show(s, MAP[i].kind == 'C' ? C_FIELDS[MAP[i].f].name : T_FIELDS[MAP[i].f].name, t);
        return;
    }
    switch (p) {
    case P_MODFX_SW: {
        const int iv = (int)lrintf(v);
        s->pv[p] = v;
        if (iv == 2) {   /* SELECT: next type (SHIFT: next subtype); the lever springs back to ON */
            if (s->ui.shift) { item_t it = {"", 'S', 0}; item_step(s, &it, 1); }
            else cset(s, C_MFX_TYPE, (s->prog.c[C_MFX_TYPE] + 1) & 3);
            cset(s, C_MFX_ON, 1);
            pv_set(s, P_MODFX_SW, 1);
            s->ui.ctx = CTX_MFX; s->ui.ctxUntil = s->clock + SEC(3);
            snprintf(t, sizeof t, "MOD FX %s", MFX_NAMES[s->prog.c[C_MFX_TYPE] & 3]);
            show(s, t, mfx_sub_name(&s->prog));
        } else {
            cset(s, C_MFX_ON, iv);
            show(s, "Mod Effect", iv ? "On" : "Off");
        }
        return;
    }
    case P_DLRV_SW: {
        const int iv = (int)lrintf(v);
        s->pv[p] = v;
        cset(s, C_DR_ON, iv > 0);
        if (iv) {
            cset(s, C_DR_TYPE, iv);
            s->ui.ctx = CTX_DLRV; s->ui.ctxUntil = s->clock + SEC(3);
            show(s, iv == 1 ? "DELAY" : "REVERB", iv == 1 ? DLY_SUB[s->prog.c[C_DLY_TYPE] % 12] : REV_SUB[s->prog.c[C_REV_TYPE] % 10]);
        } else show(s, "Delay/Reverb", "Off");
        return;
    }
    case P_VALUE: {
        float d = v - s->ui.lastValue;
        s->ui.lastValue = v;
        s->pv[p] = v;
        s->ui.lastValueClock = s->clock;
        if (fabsf(d) > 0.4f) return;   /* the spring-back, or a jump: not a turn */
        s->ui.valueAcc += d * 100.f;   /* one step per 1 % of travel */
        while (s->ui.valueAcc >= 1.f) { s->ui.valueAcc -= 1.f; value_steps(s, 1); }
        while (s->ui.valueAcc <= -1.f) { s->ui.valueAcc += 1.f; value_steps(s, -1); }
        return;
    }
    case P_SHIFT:
        s->ui.shift = v >= 0.5f; s->pv[p] = v;
        sync_multi(s);
        s->rev_++;
        return;
    case P_LFC_SW: s->g.lfcOn = on; s->pv[p] = v; show(s, "L.F. Comp", on ? "On" : "Off"); return;
    case P_LFC_GAIN: s->g.lfcGain = clampf(v, 0, 1); s->pv[p] = v; snprintf(t, sizeof t, "%.0f", v * 100.f); show(s, "Comp Gain", t); return;
    case P_MASTER: s->g.master = clampf(v, 0, 1); s->pv[p] = v; snprintf(t, sizeof t, "%.0f %%", v * 100.f); show(s, "Master", t); return;
    case P_BEND: s->bend = clampf(v, -1, 1); s->pv[p] = v; return;
    case P_MODWHEEL: s->modwheel = clampf(v, 0, 127); s->pv[p] = v; return;
    case P_TEMPO: s->pv[p] = v; cset(s, C_TEMPO, (int)lrintf(v * 10.f)); snprintf(t, sizeof t, "%.0f BPM", v); show(s, "Tempo", t); return;
    case P_MULTI_SEL: {
        const int mt = s->prog.t[et][T_M_TYPE], iv = (int)lrintf(v);
        s->pv[p] = v;
        tset(s, multi_sel_field(mt), mt == 0 ? (iv > 3 ? 3 : iv) : iv);
        if (mt == 0 && iv > 3) pv_set(s, P_MULTI_SEL, 3);
        sync_multi(s);
        panel_text(s, P_MULTI_SEL, t, sizeof t);
        show(s, mt == 0 ? "Noise Type" : mt == 1 ? "VPM Type" : "User Type", t);
        return;
    }
    case P_MULTI_SHAPE: {
        const int mt = s->prog.t[et][T_M_TYPE];
        s->pv[p] = v;
        tset(s, multi_shape_field(mt, s->ui.shift), (int)lrintf(v));
        panel_text(s, P_MULTI_SHAPE, t, sizeof t);
        show(s, s->ui.shift && mt ? "Shift Shape" : "Multi Shape", t);
        return;
    }
    case P_PROGRAM: s->pv[p] = v; if ((int)lrintf(v) != s->progNum) load_program(s, (int)lrintf(v)); return;
    default: break;
    }
    /* buttons: act on the press */
    s->pv[p] = 0;
    if (!button_press(s, p, on)) return;
    if (p >= P_PGM1 && p <= P_PGM8) { fav_or_page(s, p - P_PGM1); return; }
    switch (p) {
    case P_EDIT:
        if (s->ui.shift) { s->ui.global = !s->ui.global; s->ui.edit = 0; s->ui.shift = 0; pv_set(s, P_SHIFT, 0); }
        else { s->ui.edit = !s->ui.edit; s->ui.global = 0; }
        s->ui.page = 0; s->ui.item = 0; s->rev_++;
        break;
    case P_EXIT: s->ui.edit = s->ui.global = 0; s->ui.ctx = CTX_NONE; s->ui.msgUntil = 0; s->rev_++; break;
    case P_WRITE:
        s->bank[s->progNum] = s->prog;
        s->written[s->progNum] = 1;
        bank_save(s);
        s->ui.dirty = 0;
        program_name(s, s->progNum, t, sizeof t);
        show(s, "Program written", t);
        break;
    case P_OCT_DN: case P_OCT_UP:
        cset(s, C_OCTAVE, s->prog.c[C_OCTAVE] + (p == P_OCT_UP ? 1 : -1));
        snprintf(t, sizeof t, "%+d", s->prog.c[C_OCTAVE]);
        show(s, "Octave", t);
        break;
    case P_VM_SELECT:
        tset(s, T_VM_TYPE, (s->prog.t[et][T_VM_TYPE] + 1) & 3);
        synth_all_off(s, 0);
        pv_set(s, P_VM_TYPE, s->prog.t[et][T_VM_TYPE]);
        tfield_text(s, et, T_VM_DEPTH, s->prog.t[et][T_VM_DEPTH], t, sizeof t);
        show(s, VM_NAMES[s->prog.t[et][T_VM_TYPE] & 3], t);
        break;
    case P_ARP_BTN: {   /* ON/LATCH: Off -> On -> Latch -> Off */
        const int m = (s->prog.c[C_ARP_MODE] + 1) % 3;
        if (!m) synth_all_off(s, 0);
        cset(s, C_ARP_MODE, m);
        pv_set(s, P_ARP_MODE, m);
        show(s, "Arpeggiator", m == 0 ? "Off" : m == 1 ? "On" : "Latch");
        break;
    }
    case P_ARP_TEMPO_BTN:
        s->ui.ctx = s->ui.ctx == CTX_TEMPO ? CTX_NONE : CTX_TEMPO; s->ui.ctxUntil = s->clock + SEC(3);
        snprintf(t, sizeof t, "%.1f BPM%s", synth_tempo(s), s->host.tempo > 1 ? " (MPC)" : "");
        show(s, "Tempo (VALUE)", t);
        break;
    case P_ARP_RANGE_BTN:
        cset(s, C_ARP_RANGE, (s->prog.c[C_ARP_RANGE] + 1) & 3);
        pv_set(s, P_ARP_RANGE, s->prog.c[C_ARP_RANGE]);
        snprintf(t, sizeof t, "%d Oct", s->prog.c[C_ARP_RANGE] + 1);
        show(s, "Arp Range", t);
        break;
    case P_ARP_TYPE_BTN:
        cset(s, C_ARP_TYPE, (s->prog.c[C_ARP_TYPE] + 1) % 6);
        pv_set(s, P_ARP_TYPE, s->prog.c[C_ARP_TYPE]);
        cfield_text(s, C_ARP_TYPE, s->prog.c[C_ARP_TYPE], t, sizeof t);
        show(s, "Arp Type", t);
        break;
    default: break;
    }
}

int panel_led(synth_t *s, int p)
{
    if (p >= P_PGM1 && p <= P_PGM8) {
        const int b = p - P_PGM1;
        if (s->ui.edit || s->ui.global) return s->ui.page == b;
        return s->ui.fav[b] == s->progNum;
    }
    switch (p) {
    case P_EDIT: return s->ui.edit || s->ui.global;
    case P_OCT_DN: return s->prog.c[C_OCTAVE] < 0;
    case P_OCT_UP: return s->prog.c[C_OCTAVE] > 0;
    case P_ARP_BTN: return s->prog.c[C_ARP_MODE] > 0;
    case P_ARP_TEMPO_BTN: return s->ui.ctx == CTX_TEMPO;
    case P_WRITE: return s->ui.dirty;
    default: return 0;
    }
}

void panel_oled(synth_t *s, int line, char *b, int n)
{
    const item_t *items;
    item_t tmp[PL_NAME];
    char name[40];
    if (s->ui.edit) {
        const int ni = page_items(s, s->ui.page, &items, tmp);
        if (!line) { snprintf(b, (size_t)n, "EDIT %d  %s", s->ui.page + 1, PAGE_NAMES[s->ui.page]); return; }
        if (ni <= 0) { snprintf(b, (size_t)n, "No parameters"); return; }
        {
            const item_t *it = &items[s->ui.item % ni];
            char v[40];
            item_text(s, it, v, sizeof v);
            if (it->kind == 'N') snprintf(b, (size_t)n, "%s", v);
            else snprintf(b, (size_t)n, "%s: %s", it->name, v);
        }
        return;
    }
    if (s->ui.global) {
        char v[40];
        if (s->ui.page > 1) s->ui.page = 0;
        gl_text(s, s->ui.page, s->ui.item % gl_items(s->ui.page), name, v, sizeof v);
        if (!line) snprintf(b, (size_t)n, "%s", GPAGE_NAMES[s->ui.page]);
        else snprintf(b, (size_t)n, "%s: %s", name, v);
        return;
    }
    if ((int32_t)(s->ui.msgUntil - s->clock) > 0) { snprintf(b, (size_t)n, "%s", line ? s->ui.msg2 : s->ui.msg1); return; }
    if (!line) { program_name(s, s->progNum, b, n); if (s->ui.dirty && (int)strlen(b) + 2 < n) strcat(b, " *"); return; }
    {
        const int16_t *t = s->prog.t[0];
        char d[32];
        tfield_text(s, 0, T_VM_DEPTH, t[T_VM_DEPTH], d, sizeof d);
        snprintf(b, (size_t)n, "%s %s  Oct %+d%s", VM_NAMES[t[T_VM_TYPE] & 3], t[T_VM_TYPE] ? d : "",
                 s->prog.c[C_OCTAVE], s->prog.c[C_SUB_ON] ? "  +SUB" : "");
    }
}

void panel_tick(synth_t *s)
{
    /* the VALUE knob springs back to the middle 0.3 s after the last move */
    if (s->pv[P_VALUE] != 0.5f && s->clock - s->ui.lastValueClock > SEC(0.3)) {
        s->pv[P_VALUE] = 0.5f; s->ui.lastValue = 0.5f; s->ui.valueAcc = 0; s->rev_++;
    }
    if (s->ui.ctx && (int32_t)(s->ui.ctxUntil - s->clock) <= 0) s->ui.ctx = CTX_NONE;
    if (fabsf(s->pv[P_LFC_VU] - s->vu) > 0.004f) { s->pv[P_LFC_VU] = s->vu; s->rev_++; }
    {   /* message timeouts change the OLED text */
        static const uint32_t B = (uint32_t)PL_BLOCK;
        if (s->ui.msgUntil && (int32_t)(s->ui.msgUntil - s->clock) <= 0 && (int32_t)(s->ui.msgUntil - s->clock) > -(int32_t)B) s->rev_++;
    }
}

void panel_init(synth_t *s)
{
    int i;
    memset(&s->ui, 0, sizeof s->ui);
    for (i = 0; i < 8; i++) s->ui.fav[i] = i;
    s->ui.lastValue = 0.5f;
    s->pv[P_VALUE] = 0.5f;
    s->pv[P_LFC_VU] = 0.708f;
    panel_sync(s);
}

/* ---------------------------------------------------------------- program text ---- */
static void prog_write(FILE *f, const program_t *p)
{
    int i, t;
    fprintf(f, "name=%s\n", p->name);
    for (i = 0; i < C_COUNT; i++) fprintf(f, "c.%s=%d\n", C_FIELDS[i].key, p->c[i]);
    for (t = 0; t < 2; t++) for (i = 0; i < T_COUNT; i++) fprintf(f, "t%d.%s=%d\n", t, T_FIELDS[i].key, p->t[t][i]);
}

static int prog_line(program_t *p, const char *line)
{
    char k[40];
    int v, i;
    if (!strncmp(line, "name=", 5)) {
        size_t l;
        snprintf(p->name, sizeof p->name, "%s", line + 5);
        l = strlen(p->name);
        while (l && (p->name[l - 1] == '\n' || p->name[l - 1] == '\r')) p->name[--l] = 0;
        return 1;
    }
    if (sscanf(line, "c.%39[^=]=%d", k, &v) == 2) {
        for (i = 0; i < C_COUNT; i++) if (!strcmp(C_FIELDS[i].key, k)) { p->c[i] = (int16_t)clampf((float)v, C_FIELDS[i].min, C_FIELDS[i].max); return 1; }
        return 1;
    }
    if (line[0] == 't' && (line[1] == '0' || line[1] == '1') && line[2] == '.' && sscanf(line + 3, "%39[^=]=%d", k, &v) == 2) {
        const int t = line[1] - '0';
        for (i = 0; i < T_COUNT; i++) if (!strcmp(T_FIELDS[i].key, k)) { p->t[t][i] = (int16_t)clampf((float)v, T_FIELDS[i].min, T_FIELDS[i].max); return 1; }
        return 1;
    }
    return 0;
}

void bank_load(synth_t *s)
{
    char path[600], line[200];
    FILE *f;
    int i, cur = -1;
    const int nf = prog_factory_count();
    for (i = 0; i < PL_PROGRAMS; i++) {
        if (i < nf) prog_factory(&s->bank[i], i);
        else prog_init(&s->bank[i], "Init Program");
        s->written[i] = 0;
    }
    if (!s->dataDir[0]) return;
    snprintf(path, sizeof path, "%s/programs.txt", s->dataDir);
    if (!(f = fopen(path, "r"))) return;
    while (fgets(line, sizeof line, f)) {
        int n;
        if (sscanf(line, "[program %d]", &n) == 1 && n >= 1 && n <= PL_PROGRAMS) {
            cur = n - 1; prog_init(&s->bank[cur], NULL); s->written[cur] = 1; continue;
        }
        if (cur >= 0) prog_line(&s->bank[cur], line);
    }
    fclose(f);
}

void bank_save(synth_t *s)
{
    char path[600], tmp[620];
    FILE *f;
    int i;
    if (!s->dataDir[0]) return;
    snprintf(path, sizeof path, "%s/programs.txt", s->dataDir);
    snprintf(tmp, sizeof tmp, "%s.new", path);
    if (!(f = fopen(tmp, "w"))) return;
    fprintf(f, "# PrLogueXXL user programs (only the slots written with WRITE)\n");
    for (i = 0; i < PL_PROGRAMS; i++) if (s->written[i]) { fprintf(f, "[program %d]\n", i + 1); prog_write(f, &s->bank[i]); }
    fclose(f);
    rename(tmp, path);
}

/* ---------------------------------------------------------------- plugin state ---- */
char *state_save(synth_t *s, int *len)
{
    char *buf = NULL;
    size_t sz = 0;
    FILE *f = open_memstream(&buf, &sz);
    int i;
    if (!f) return NULL;
    fprintf(f, "PRLOGUEXXL 1\n");
    fprintf(f, "g.program=%d\ng.master_tune=%d\ng.transpose=%d\ng.vel_curve=%d\ng.lfc_on=%d\ng.lfc_gain=%.4f\ng.master=%.4f\ng.dirty=%d\n",
            s->progNum, s->g.masterTune, s->g.transpose, s->g.velCurve, s->g.lfcOn, s->g.lfcGain, s->g.master, s->ui.dirty);
    for (i = 0; i < 8; i++) fprintf(f, "g.fav%d=%d\n", i, s->ui.fav[i]);
    prog_write(f, &s->prog);
    fclose(f);
    *len = (int)sz;
    return buf;
}

int state_load(synth_t *s, const char *txt)
{
    const char *p = txt;
    char line[200];
    program_t pr;
    if (strncmp(txt, "PRLOGUEXXL", 10)) return -1;
    prog_init(&pr, NULL);
    while (*p) {
        const char *e = strchr(p, '\n');
        size_t l = e ? (size_t)(e - p) : strlen(p);
        float fv;
        int iv;
        if (l >= sizeof line) l = sizeof line - 1;
        memcpy(line, p, l); line[l] = 0;
        p = e ? e + 1 : p + l;
        if (sscanf(line, "g.program=%d", &iv) == 1) s->progNum = (int)clampf((float)iv, 0, PL_PROGRAMS - 1);
        else if (sscanf(line, "g.master_tune=%d", &iv) == 1) s->g.masterTune = iv;
        else if (sscanf(line, "g.transpose=%d", &iv) == 1) s->g.transpose = iv;
        else if (sscanf(line, "g.vel_curve=%d", &iv) == 1) s->g.velCurve = iv;
        else if (sscanf(line, "g.lfc_on=%d", &iv) == 1) s->g.lfcOn = iv;
        else if (sscanf(line, "g.lfc_gain=%f", &fv) == 1) s->g.lfcGain = fv;
        else if (sscanf(line, "g.master=%f", &fv) == 1) s->g.master = fv;
        else if (sscanf(line, "g.dirty=%d", &iv) == 1) s->ui.dirty = iv;
        else if (!strncmp(line, "g.fav", 5)) { int k; if (sscanf(line, "g.fav%d=%d", &k, &iv) == 2 && k >= 0 && k < 8) s->ui.fav[k] = iv; }
        else prog_line(&pr, line);
    }
    synth_all_off(s, 1);
    s->prog = pr;
    user_params_apply(s);
    panel_sync(s);
    return 0;
}
