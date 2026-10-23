/* PrLogueXXL -- the plugin engine (mpc-vst-plugins wrapper/engine.h): MPC's 44.1 kHz int16 blocks, parameters,
 * MIDI and song position, over the synth (synth.c) and its panel (panel.c).
 *
 * Data folder: /sdcard/PrLogueXXL (MODULE_DIR), else "<plugin folder>/PrLogueXXL Data": programs.txt holds the
 * programs written with WRITE.
 * MIDI: notes, velocity, pitch bend, CC1 mod wheel, CC64 damper, channel pressure (AFTER TOUCH assign), program
 * change (+ bank select LSB 0-4 = programs 1-100, 101-200 ..), CC120/123, and KORG's prologue CC chart
 * (docs/PROLOGUE_SPEC.md section 2). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "engine.h"
#include "plugin_dir.h"
#include "synthint.h"

static int writable_dir(const char *d)
{
    char tmp[600], probe[620];
    char *p;
    FILE *f;
    snprintf(tmp, sizeof tmp, "%s", d);
    for (p = tmp + 1; *p; p++) if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    mkdir(tmp, 0755);
    snprintf(probe, sizeof probe, "%s/.probe", d);
    if (!(f = fopen(probe, "w"))) return 0;
    fclose(f);
    remove(probe);
    return 1;
}

static void *create(const char *dataDir)
{
    synth_t *s = calloc(1, sizeof *s);
    char here[512] = "";
    int i;
    if (!s) return NULL;
    s->bank = calloc(PL_PROGRAMS, sizeof(program_t));
    if (!s->bank || fx_alloc(&s->mfx, &s->dly, &s->rev)) { fx_free(&s->mfx, &s->dly, &s->rev); free(s->bank); free(s); return NULL; }
    mpc_plugin_dir(here, sizeof here);
    if (dataDir && writable_dir(dataDir)) snprintf(s->dataDir, sizeof s->dataDir, "%s", dataDir);
    else if (here[0]) { snprintf(s->dataDir, sizeof s->dataDir, "%s/PrLogueXXL Data", here); if (!writable_dir(s->dataDir)) s->dataDir[0] = 0; }
    for (i = 0; i < PL_VOICES; i++) s->v[i].idx = i;
    s->rng.s = 0xC0FFEEu;
    s->arp.sounding = s->arp.sounding2 = -1;
    s->arp.lastHostStep = -1;
    s->g.master = 0.7f; s->g.lfcGain = 0.3f; s->g.velCurve = 3;
    s->vu = 0.708f;
    user_osc_init();
    bank_load(s);
    panel_init(s);
    load_program(s, 0);
    return s;
}

static void destroy(void *p)
{
    synth_t *s = p;
    if (!s) return;
    fx_free(&s->mfx, &s->dly, &s->rev);
    free(s->bank);
    free(s);
}

/* ---------------------------------------------------------------- MIDI ---- */
/* KORG's CC chart (MIDI implementation): continuous CCs -> 10-bit fields of the edited timbre, or common fields */
typedef struct { uint8_t cc; char kind; int16_t f; } ccmap_t;
static const ccmap_t CC[] = {
    {5, 'T', T_PORTA}, {8, 'C', C_BALANCE}, {14, 'T', T_SPREAD}, {16, 'T', T_AA}, {17, 'T', T_AD}, {18, 'T', T_AS},
    {19, 'T', T_AR}, {20, 'T', T_EA}, {21, 'T', T_ED}, {22, 'T', T_ES}, {23, 'T', T_ER}, {24, 'T', T_LFO_RATE},
    {26, 'T', T_LFO_INT}, {27, 'T', T_VM_DEPTH}, {28, 'C', C_MFX_SPEED}, {29, 'C', C_MFX_DEPTH}, {30, 'C', C_DR_TIME},
    {31, 'C', C_DR_DEPTH}, {33, 'T', T_LVM}, {34, 'T', T_V1_PITCH}, {35, 'T', T_V2_PITCH}, {36, 'T', T_V1_SHAPE},
    {37, 'T', T_V2_SHAPE}, {39, 'T', T_LV1}, {40, 'T', T_LV2}, {41, 'T', T_XMOD}, {42, 'T', T_PEG_INT}, {43, 'T', T_CUTOFF},
    {44, 'T', T_RESO}, {45, 'T', T_CUT_EG}, {111, 'C', C_DRYWET},
    /* switches (KORG's receive bands) */
    {48, 'T', T_V1_OCT}, {49, 'T', T_V2_OCT}, {50, 'T', T_V1_WAVE}, {51, 'T', T_V2_WAVE}, {52, 'T', T_M_OCT}, {53, 'T', T_M_TYPE},
    {56, 'T', T_LFO_TGT}, {57, 'T', T_LFO_WAVE}, {58, 'T', T_LFO_MODE}, {80, 'T', T_SYNC_RING}, {81, 'T', T_PEG_TGT},
    {82, 'T', T_LOWCUT}, {83, 'T', T_KEYTRK}, {84, 'T', T_DRIVE}, {85, 'C', C_EDIT_TIMBRE}, {86, 'C', C_TIMBRE_TYPE},
    {92, 'C', C_MFX_ON}, {94, 'C', C_DR_ON},
};

static int bands(int v, int n) { int i = v * n / 128; return i >= n ? n - 1 : i; }

static void midi_cc(synth_t *s, int cc, int v)
{
    unsigned i;
    switch (cc) {
    case 1: s->modwheel = (float)v; s->pv[P_MODWHEEL] = (float)v; s->rev_++; return;
    case 64: synth_damper(s, v >= 64); return;
    case 120: synth_all_off(s, 1); return;
    case 123: synth_all_off(s, 0); return;
    case 32: s->bankLsb = v > 4 ? 4 : v; return;
    case 54: case 104: {   /* multi shape / shift shape */
        const int t = s->prog.c[C_EDIT_TIMBRE] == 2, mt = s->prog.t[t][T_M_TYPE];
        const int f = mt == 0 ? T_SH_NOISE : mt == 1 ? (cc == 104 ? T_SSH_VPM : T_SH_VPM) : (cc == 104 ? T_SSH_USER : T_SH_USER);
        if (cc == 104 && mt == 0) return;
        s->prog.t[t][f] = (int16_t)(v * 1023 / 127);
        if (s->prog.c[C_EDIT_TIMBRE] == 1) s->prog.t[1][f] = s->prog.t[0][f];
        panel_sync(s);
        return;
    }
    case 89: s->prog.c[C_DR_TYPE] = (int16_t)(v >= 64 ? 2 : 1); panel_sync(s); return;
    case 88: s->prog.c[C_MFX_TYPE] = (int16_t)bands(v, 4); panel_sync(s); return;
    default: break;
    }
    for (i = 0; i < sizeof CC / sizeof CC[0]; i++) {
        int f = CC[i].f, val;
        if (CC[i].cc != cc) continue;
        if (CC[i].kind == 'C') {
            const field_t *d = &C_FIELDS[f];
            if (f == C_MFX_ON || f == C_DR_ON) val = v >= 64;
            else if (f == C_EDIT_TIMBRE) val = 2 - bands(v, 3);   /* SUB, +, MAIN */
            else if (f == C_TIMBRE_TYPE) val = bands(v, 3);
            else val = d->min + (int)lroundf(v / 127.f * (float)(d->max - d->min));
            s->prog.c[f] = (int16_t)val;
        } else {
            const field_t *d = &T_FIELDS[f];
            const int range = d->max - d->min;
            if (range <= 3) {
                val = bands(v, range + 1);
            } else val = d->min + (int)lroundf(v / 127.f * (float)range);
            if (s->prog.c[C_EDIT_TIMBRE] != 2) s->prog.t[0][f] = (int16_t)val;
            if (s->prog.c[C_EDIT_TIMBRE] != 0) s->prog.t[1][f] = (int16_t)val;
        }
        s->ui.dirty = 1;
        panel_sync(s);
        return;
    }
}

static void midi(void *p, const uint8_t *msg, int len)
{
    synth_t *s = p;
    const int st = msg[0] & 0xF0;
    if (len < 1) return;
    if (st == 0x90 && len >= 3 && msg[2]) synth_note_on(s, msg[1] & 127, msg[2] & 127);
    else if (st == 0x80 || (st == 0x90 && len >= 3 && !msg[2])) synth_note_off(s, msg[1] & 127);
    else if (st == 0xB0 && len >= 3) midi_cc(s, msg[1] & 127, msg[2] & 127);
    else if (st == 0xE0 && len >= 3) {
        const int b = ((msg[2] & 127) << 7 | (msg[1] & 127)) - 8192;
        s->bend = b >= 0 ? b / 8191.f : b / 8192.f;
        s->pv[P_BEND] = s->bend; s->rev_++;
    } else if (st == 0xD0 && len >= 2) s->aftertouch = (float)(msg[1] & 127);
    else if (st == 0xC0 && len >= 2) { load_program(s, (s->bankLsb * 100 + (msg[1] & 127)) % PL_PROGRAMS); }
}

/* ---------------------------------------------------------------- parameters ---- */
static int key_index(const char *k, const char **sfx)
{
    int i, best = -1;
    size_t bl = 0;
    for (i = 0; i < P_COUNT; i++) {
        const size_t n = strlen(PARAM_KEYS[i]);
        if (!strncmp(k, PARAM_KEYS[i], n) && (k[n] == 0 || k[n] == '_' || k[n] == ':') && n > bl) { best = i; bl = n; }
    }
    *sfx = best >= 0 ? k + bl : "";
    return best;
}

static int put(char *buf, int n, const char *s)
{
    const int l = (int)strlen(s);
    if (n <= 0) return 0;
    snprintf(buf, (size_t)n, "%s", s[0] ? s : " ");
    return l > 0 ? l : 1;
}

static void set_param(void *p, const char *k, const char *v)
{
    synth_t *s = p;
    const char *sfx;
    int i;
    if (!strcmp(k, "state")) { state_load(s, v); return; }
    if (!strcmp(k, "host_time")) {
        int playing = 0;
        double ppq = 0, tempo = 0;
        if (sscanf(v, "%d %lf %lf", &playing, &ppq, &tempo) == 3) {
            if (s->host.playing && !playing) s->arp.lastHostStep = -1;
            s->host.playing = playing; s->host.ppq = ppq; s->host.tempo = tempo;
        }
        return;
    }
    i = key_index(k, &sfx);
    if (i < 0 || *sfx) return;
    panel_set(s, i, (float)atof(v));
}

static int get_param(void *p, const char *k, char *buf, int n)
{
    synth_t *s = p;
    const char *sfx;
    char t[96];
    int i;
    if (!strcmp(k, "state")) {
        int len = 0, r = 0;
        char *st = state_save(s, &len);
        if (st) { if (len < n) memcpy(buf, st, (size_t)len + 1); r = len + 1; free(st); }
        return r;
    }
    if (!strcmp(k, "display_rev")) { snprintf(t, sizeof t, "%u", (unsigned)s->rev_); return put(buf, n, t); }
    i = key_index(k, &sfx);
    if (i < 0) return 0;
    if (i == P_PROGRAM && sfx[0] == ':') { program_name(s, atoi(sfx + 1), t, sizeof t); return put(buf, n, t + 4); }
    if (i == P_OLED1 || i == P_OLED2) { if (*sfx) return 0; panel_oled(s, i - P_OLED1, t, sizeof t); return put(buf, n, t); }
    if (!strcmp(sfx, "_on")) return put(buf, n, panel_led(s, i) ? "1" : "0");
    if (!strcmp(sfx, "_display")) { if (!panel_text(s, i, t, sizeof t)) return 0; return put(buf, n, t); }
    if (*sfx) return 0;
    if (PARAM_MAX[i] - PARAM_MIN[i] >= 2 || i == P_SHIFT || i == P_LFC_SW || i == P_SUB_ON || i == P_LOW_CUT)
        snprintf(t, sizeof t, "%d", (int)lrintf(s->pv[i]));
    else snprintf(t, sizeof t, "%.5f", s->pv[i]);
    if (i == P_VALUE || i == P_MASTER || i == P_LFC_GAIN || i == P_LFC_VU || i == P_BEND) snprintf(t, sizeof t, "%.5f", s->pv[i]);
    return put(buf, n, t);
}

/* ---------------------------------------------------------------- audio ---- */
static void render(void *p, int16_t *out, int frames)
{
    synth_t *s = p;
    float L[PL_BLOCK], R[PL_BLOCK];
    int done = 0, i;
    while (done < frames) {
        const int n = frames - done > PL_BLOCK ? PL_BLOCK : frames - done;
        synth_render(s, L, R, n);
        for (i = 0; i < n; i++) {
            float l = L[i], r = R[i];
            /* output stage: a soft limit just below full scale */
            l = l > 0.9f || l < -0.9f ? sat(l) : l;
            r = r > 0.9f || r < -0.9f ? sat(r) : r;
            out[2 * (done + i)] = (int16_t)lrintf(clampf(l, -1.f, 1.f) * 32767.f);
            out[2 * (done + i) + 1] = (int16_t)lrintf(clampf(r, -1.f, 1.f) * 32767.f);
        }
        done += n;
    }
    panel_tick(s);
}

static const mpc_engine_t ENGINE = {create, destroy, midi, set_param, get_param, render, NULL, NULL};
const mpc_engine_t *mpc_engine(void) { return &ENGINE; }
