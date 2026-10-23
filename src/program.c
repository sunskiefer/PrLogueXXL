/* PrLogueXXL -- program data: field tables (KORG's ranges), KORG's value curves, init and factory programs. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "pl.h"

/* key, menu name, min, max, default */
const field_t T_FIELDS[T_COUNT] = {
    {"porta", "Portamento", 0, 128, 0}, {"spread", "Voice Spread", 0, 127, 0},
    {"vm_depth", "Voice Mode Depth", 0, 1023, 0}, {"vm_type", "Voice Mode", 0, 3, 0},
    {"v1_wave", "VCO1 Wave", 0, 2, 2}, {"v1_oct", "VCO1 Octave", 0, 3, 2}, {"v1_pitch", "VCO1 Pitch", 0, 1023, 512},
    {"v1_shape", "VCO1 Shape", 0, 1023, 0}, {"peg_tgt", "Pitch EG Target", 0, 2, 0}, {"peg_int", "Pitch EG Int", 0, 1023, 512},
    {"v2_wave", "VCO2 Wave", 0, 2, 2}, {"v2_oct", "VCO2 Octave", 0, 3, 2}, {"v2_pitch", "VCO2 Pitch", 0, 1023, 512},
    {"v2_shape", "VCO2 Shape", 0, 1023, 0}, {"sync_ring", "Sync/Ring", 0, 2, 1}, {"xmod", "Cross Mod", 0, 1023, 0},
    {"m_route", "Multi Routing", 0, 1, 0}, {"m_type", "Multi Type", 0, 2, 0}, {"m_oct", "Multi Octave", 0, 3, 2},
    {"sel_noise", "Noise Type", 0, 3, 0}, {"sel_vpm", "VPM Type", 0, 15, 0}, {"sel_user", "User Type", 0, 15, 0},
    {"sh_noise", "Noise Shape", 0, 1023, 512}, {"sh_vpm", "VPM Shape", 0, 1023, 256}, {"ssh_vpm", "VPM Shift Shape", 0, 1023, 300},
    {"sh_user", "User Shape", 0, 1023, 0}, {"ssh_user", "User Shift Shape", 0, 1023, 0},
    {"lv1", "VCO1 Level", 0, 1023, 1023}, {"lv2", "VCO2 Level", 0, 1023, 0}, {"lvm", "Multi Level", 0, 1023, 0},
    {"cutoff", "Cutoff", 0, 1023, 1023}, {"reso", "Resonance", 0, 1023, 0}, {"cut_eg", "Cutoff EG Int", 0, 1023, 512},
    {"drive", "Drive", 0, 2, 0}, {"lowcut", "Low Cut", 0, 1, 0}, {"keytrk", "Keytrack", 0, 2, 0},
    {"cut_vel", "EG Velocity", 0, 127, 0},
    {"aa", "Amp Attack", 0, 1023, 0}, {"ad", "Amp Decay", 0, 1023, 512}, {"as", "Amp Sustain", 0, 1023, 1023},
    {"ar", "Amp Release", 0, 1023, 300},
    {"ea", "EG Attack", 0, 1023, 0}, {"ed", "EG Decay", 0, 1023, 512}, {"es", "EG Sustain", 0, 1023, 0},
    {"er", "EG Release", 0, 1023, 300},
    {"lfo_wave", "LFO Wave", 0, 2, 1}, {"lfo_mode", "LFO Mode", 0, 2, 1}, {"lfo_rate", "LFO Rate", 0, 1023, 512},
    {"lfo_int", "LFO Int", -511, 511, 0}, {"lfo_tgt", "LFO Target", 0, 2, 2},
    {"mw_assign", "M.Wheel Assign", 0, 31, 26}, {"bend_up", "Bend Range +", 0, 12, 2}, {"bend_dn", "Bend Range -", 0, 12, 2},
    {"vpm_p1", "Feedback", -100, 100, 0}, {"vpm_p2", "Noise Depth", -100, 100, 0}, {"vpm_p3", "Shape Mod Int", -100, 100, 0},
    {"vpm_p4", "Mod Attack", -100, 100, 0}, {"vpm_p5", "Mod Decay", -100, 100, 0}, {"vpm_p6", "Mod KeyTrack", -100, 100, 0},
    {"usr_p1", "User Param 1", 0, 200, 0}, {"usr_p2", "User Param 2", 0, 200, 0}, {"usr_p3", "User Param 3", 0, 200, 0},
    {"usr_p4", "User Param 4", 0, 200, 0}, {"usr_p5", "User Param 5", 0, 200, 0}, {"usr_p6", "User Param 6", 0, 200, 0},
    {"mw_range", "M.Wheel Range", -100, 100, 100}, {"lfo_ksync", "LFO Key Sync", 0, 1, 0},
    {"lfo_vsync", "LFO Voice Sync", 0, 1, 0}, {"lfo_tosc", "LFO Target OSC", 0, 3, 0}, {"legato", "EG Legato", 0, 1, 0},
    {"at_assign", "AfterTouch Assign", 0, 31, 14},
};

const field_t C_FIELDS[C_COUNT] = {
    {"octave", "Octave", -2, 2, 0}, {"sub_on", "Sub Timbre", 0, 1, 0}, {"edit_timbre", "Edit Timbre", 0, 2, 0},
    {"timbre_type", "Timbre Type", 0, 2, 0}, {"balance", "Main/Sub Balance", 0, 127, 64},
    {"position", "Main/Sub Position", 0, 1, 0}, {"split", "Split Point", 0, 127, 60}, {"tempo", "Tempo", 300, 6000, 1200},
    {"arp_tgt", "Arp Target", 0, 2, 0}, {"amp_vel", "Amp Velocity", 0, 127, 64}, {"porta_mode", "Portamento Mode", 0, 1, 0},
    {"level", "Program Level", 12, 132, 102},
    {"mfx_type", "Mod FX Type", 0, 3, 0}, {"mfx_speed", "Mod FX Speed", 0, 1023, 400}, {"mfx_depth", "Mod FX Depth", 0, 1023, 500},
    {"mfx_cho", "Chorus Type", 0, 7, 0}, {"mfx_ens", "Ensemble Type", 0, 2, 0}, {"mfx_pha", "Phaser Type", 0, 7, 0},
    {"mfx_fla", "Flanger Type", 0, 7, 0},
    {"tuning", "Micro Tuning", 0, 5, 0}, {"scale_key", "Scale Key", -12, 12, 0}, {"prg_tune", "Program Tuning", -50, 50, 0},
    {"transpose", "Program Transpose", -12, 12, 0}, {"arp_gate", "Arp Gate Time", 0, 72, 54}, {"arp_rate", "Arp Rate", 0, 10, 4},
    {"drywet", "DL/RV Dry Wet", 0, 1024, 512}, {"dr_type", "DL/RV Type", 0, 2, 0}, {"dr_time", "DL/RV Time", 0, 1023, 500},
    {"dr_depth", "DL/RV Depth", 0, 1023, 400}, {"rev_type", "Reverb Type", 0, 9, 0}, {"dly_type", "Delay Type", 0, 11, 0},
    {"mfx_route", "Mod FX Routing", 0, 2, 0}, {"dr_route", "DL/RV Routing", 0, 2, 0}, {"mfx_on", "Mod FX", 0, 1, 0},
    {"dr_on", "Delay/Reverb", 0, 1, 0}, {"arp_mode", "Arpeggiator", 0, 2, 0}, {"arp_range", "Arp Range", 0, 3, 0},
    {"arp_type", "Arp Type", 0, 5, 1}, {"category", "Category", 0, 8, 0},
};

const char *const CHORD_NAMES[14] = {"5th", "sus2", "m", "Maj", "sus4", "m7", "7", "7sus4", "Maj7", "aug", "dim", "m7b5",
                                     "mMaj7", "Maj7b5"};

void prog_init(program_t *p, const char *name)
{
    int i, t;
    memset(p, 0, sizeof *p);
    snprintf(p->name, sizeof p->name, "%s", name ? name : "Init Program");
    for (i = 0; i < C_COUNT; i++) p->c[i] = C_FIELDS[i].def;
    for (t = 0; t < 2; t++) for (i = 0; i < T_COUNT; i++) p->t[t][i] = T_FIELDS[i].def;
}

/* piecewise-linear curve through KORG's breakpoints (P16 / P17 share their shape) */
static float pw(int v, const float *x, const float *y, int n)
{
    int i;
    if (v <= x[0]) return y[0];
    for (i = 1; i < n; i++)
        if (v <= x[i]) return y[i - 1] + (y[i] - y[i - 1]) * (float)(v - x[i - 1]) / (x[i] - x[i - 1]);
    return y[n - 1];
}
float pl_pitch_cents(int v)
{
    static const float X[] = {4, 356, 476, 492, 532, 548, 668, 1020};
    static const float Y[] = {-1200, -256, -16, 0, 0, 16, 256, 1200};
    return pw(v, X, Y, 8);
}
float pl_peg_cents(int v)
{
    static const float X[] = {4, 356, 476, 492, 532, 548, 668, 1020};
    static const float Y[] = {-4800, -1024, -64, 0, 0, 64, 1024, 4800};
    return pw(v, X, Y, 8);
}
float pl_cut_eg_pct(int v)
{
    if (v <= 11) return -100.f;
    if (v < 492) return -((492.f - v) * (492.f - v) * 4641.f * 100.f) / 1073741824.f;
    if (v <= 532) return 0.f;
    if (v < 1013) return ((v - 532.f) * (v - 532.f) * 4641.f * 100.f) / 1073741824.f;
    return 100.f;
}
int pl_chord_index(int d)
{
    static const int UP[14] = {73, 146, 219, 292, 365, 438, 511, 585, 658, 731, 804, 877, 950, 1023};
    int i;
    for (i = 0; i < 14; i++) if (d <= UP[i]) return i;
    return 13;
}

/* ---------------------------------------------------------------- factory programs (PrLogueXXL's own) ---- */
typedef struct { const char *name; int cat; const char *set; } fact_t;
/* "key=value ..." over prog_init(); timbre keys apply to the Main timbre, "s." prefix to the Sub timbre */
static const fact_t FACT[] = {
    {"Analog Brass", 0, "v1_wave=2 v2_wave=2 lv2=900 v2_pitch=530 cutoff=380 reso=180 cut_eg=800 ea=120 ed=520 es=420 aa=40 ad=600 as=900 ar=360 spread=60 mfx_on=1 mfx_type=0 mfx_depth=420 dr_on=1 dr_type=2 rev_type=0 dr_time=500 dr_depth=380"},
    {"Poly Pad", 3, "v1_wave=2 v2_wave=2 v1_shape=300 lv2=900 v2_pitch=545 cutoff=420 reso=120 cut_eg=640 ea=700 ed=700 es=500 er=700 aa=640 ad=700 as=950 ar=760 lfo_tgt=1 lfo_int=160 lfo_rate=320 spread=100 mfx_on=1 mfx_type=1 dr_on=1 dr_type=2 rev_type=1 dr_time=760 dr_depth=520"},
    {"Fat Bass", 1, "vm_type=1 vm_depth=600 v1_wave=2 v2_wave=0 v2_oct=3 lv2=800 cutoff=260 reso=300 cut_eg=780 ed=380 es=0 aa=0 ad=500 as=1023 ar=120 drive=1 keytrk=1"},
    {"Sync Lead", 2, "vm_type=1 v1_wave=2 v2_wave=2 sync_ring=2 v2_pitch=820 peg_tgt=0 peg_int=700 lv1=600 lv2=1023 cutoff=700 reso=200 cut_eg=600 ea=0 ed=420 es=300 aa=0 as=1023 ar=200 porta=40 mfx_on=1 dr_on=1 dr_type=1 dly_type=6 dr_time=600 dr_depth=380"},
    {"VPM Bells", 4, "lv1=0 lvm=1023 m_type=1 sel_vpm=3 sh_vpm=420 ssh_vpm=360 cutoff=1023 aa=0 ad=700 as=0 ar=620 vpm_p5=20 dr_on=1 dr_type=2 rev_type=3 dr_time=640 dr_depth=420"},
    {"Unison Saws", 2, "vm_type=2 vm_depth=420 v1_wave=2 v2_wave=2 lv2=900 v2_oct=3 cutoff=620 reso=160 cut_eg=600 ed=500 es=500 ar=260 drive=1 mfx_on=1 mfx_type=1"},
    {"Chord Stab", 5, "vm_type=3 vm_depth=600 v1_wave=0 v1_shape=300 cutoff=480 reso=420 cut_eg=820 ed=340 es=0 ad=360 as=0 ar=300 dr_on=1 dr_type=1 dly_type=8 dr_time=700 dr_depth=400"},
    {"Ring Mod Bell", 4, "v1_wave=1 v2_wave=1 sync_ring=0 v2_pitch=880 lv1=200 lv2=1023 cutoff=900 aa=0 ad=650 as=0 ar=600 dr_on=1 dr_type=2 rev_type=0 dr_time=700 dr_depth=500"},
    {"Noise Sweep", 8, "lv1=0 lvm=1023 m_type=0 sel_noise=2 sh_noise=600 cutoff=600 reso=700 cut_eg=1000 ea=900 ed=900 es=200 er=800 aa=600 ar=800 lfo_tgt=0 lfo_int=300 lfo_rate=200"},
    {"Cross Mod Lead", 2, "vm_type=1 v1_wave=1 v2_wave=2 xmod=520 v2_pitch=700 lv1=300 lv2=1023 cutoff=640 reso=300 cut_eg=620 ed=400 es=400 porta=30"},
    {"PWM Strings", 3, "v1_wave=0 v2_wave=0 v1_shape=200 v2_shape=400 lv2=1023 v2_pitch=540 lfo_tgt=1 lfo_int=260 lfo_rate=380 lfo_tosc=1 cutoff=520 cut_eg=560 ea=520 ed=600 es=600 aa=520 ar=640 spread=90 mfx_on=1 mfx_type=1 dr_on=1 dr_type=2 rev_type=1 dr_time=700 dr_depth=460"},
    {"Duo Detune", 0, "vm_depth=760 v1_wave=2 cutoff=560 reso=120 cut_eg=620 ed=500 es=500 ar=400 mfx_on=1"},
    {"Arp Pluck", 6, "arp_mode=1 arp_type=3 arp_range=1 v1_wave=0 v1_shape=500 cutoff=420 reso=380 cut_eg=840 ed=300 es=0 ad=320 as=0 ar=280 dr_on=1 dr_type=1 dly_type=6 dr_time=500 dr_depth=420"},
    {"Wah Clav", 4, "v1_wave=0 v1_shape=700 cutoff=360 reso=600 cut_eg=880 ed=260 es=0 ad=420 as=200 ar=160 keytrk=2 mfx_on=1 mfx_type=2"},
    {"Air Choir", 3, "lv1=500 lvm=900 m_type=1 sel_vpm=10 sh_vpm=300 v1_wave=1 cutoff=600 ea=640 ed=800 es=600 aa=700 ar=800 mfx_on=1 mfx_type=1 dr_on=1 dr_type=2 rev_type=2 dr_time=800 dr_depth=560"},
    {"Split Bass/Pad", 7, "sub_on=1 timbre_type=2 split=55 position=0 v1_wave=2 v2_wave=2 lv2=900 v2_pitch=545 cutoff=480 ea=600 aa=640 ar=700 s.vm_type=1 s.v1_wave=2 s.v2_wave=0 s.v2_oct=3 s.lv2=800 s.cutoff=300 s.reso=300 s.cut_eg=760 s.ed=380 s.es=0 s.ar=120"},
    {"Layer Bell Pad", 7, "sub_on=1 timbre_type=0 balance=60 v1_wave=2 v2_wave=2 lv2=900 v2_pitch=540 cutoff=440 ea=700 aa=700 ar=760 s.lv1=0 s.lvm=1023 s.m_type=1 s.sel_vpm=2 s.sh_vpm=400 s.ad=700 s.as=0 s.ar=620 dr_on=1 dr_type=2 dr_time=700 dr_depth=500"},
    {"Decim Perc", 8, "lv1=0 lvm=1023 m_type=0 sel_noise=3 sh_noise=300 m_oct=3 aa=0 ad=300 as=0 ar=200 cutoff=800"},
    {"Fast LFO FM", 8, "v1_wave=1 lfo_mode=2 lfo_rate=600 lfo_tgt=2 lfo_int=200 cutoff=800 ar=400"},
    {"Creep Texture", 8, "lv1=0 lvm=1023 m_type=1 sel_vpm=14 sh_vpm=600 ssh_vpm=500 aa=500 ar=700 dr_on=1 dr_type=2 rev_type=6 dr_time=800 dr_depth=600"},
    {"Throat Lead", 2, "vm_type=1 lv1=400 lvm=1023 m_type=1 sel_vpm=15 sh_vpm=500 cutoff=700 reso=200 porta=50"},
    {"Waves User", 0, "lv1=0 lvm=1023 m_type=2 sel_user=0 sh_user=300 ssh_user=200 usr_p1=10 usr_p2=20 usr_p3=0 usr_p4=40 cutoff=760 ea=200 aa=200 ar=500 mfx_on=1 dr_on=1 dr_type=2"},
};

int prog_factory_count(void) { return (int)(sizeof FACT / sizeof FACT[0]); }

static int find_field(const field_t *f, int n, const char *k)
{
    int i;
    for (i = 0; i < n; i++) if (!strcmp(f[i].key, k)) return i;
    return -1;
}

void prog_factory(program_t *p, int n)
{
    const char *s;
    char k[32];
    int val, used;
    if (n < 0 || n >= prog_factory_count()) { prog_init(p, NULL); return; }
    prog_init(p, FACT[n].name);
    p->c[C_CATEGORY] = (int16_t)FACT[n].cat;
    for (s = FACT[n].set; sscanf(s, " %31[^=]=%d%n", k, &val, &used) == 2; s += used) {
        int t = 0, i;
        const char *kk = k;
        if (!strncmp(k, "s.", 2)) { t = 1; kk = k + 2; }
        if ((i = find_field(T_FIELDS, T_COUNT, kk)) >= 0) p->t[t][i] = (int16_t)val;
        else if ((i = find_field(C_FIELDS, C_COUNT, kk)) >= 0) p->c[i] = (int16_t)val;
    }
}
