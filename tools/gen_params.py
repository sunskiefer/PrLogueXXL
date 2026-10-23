#!/usr/bin/env python3
"""The single source of PrLogueXXL's plugin parameters: writes params.json (mpc-vst-plugins) and src/param_ids.h.

The prologue-16's front panel, page by page (docs/PROLOGUE_SPEC.md has KORG's ranges), plus a few values the engine
sets for the screen (OLED lines, the multi engine's 7-segment display, the L.F. COMP meter). The panel edits the timbre
the TIMBRE "Panel" switch selects, as on the prologue-16; everything else of a program (PROGRAM EDIT pages) lives in
the engine's program and is reached with the 8 PROGRAM SORT/EDIT PAGE buttons and the PROGRAM/VALUE knob.
MPC stores a project's values by parameter index, so parameters are only ever appended (params.lock checks it)."""
import json, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

OCT = ["2'", "4'", "8'", "16'"]
WAVE = ["SQR", "TRI", "SAW"]
NOISE = ["HIGH", "LOW", "PEAK", "DECIM"]
VPM = ["SIN1", "SIN2", "SIN3", "SIN4", "SAW1", "SAW2", "SQU1", "SQU2", "FAT1", "FAT2", "AIR1", "AIR2", "DECAY1",
       "DECAY2", "CREEP", "THROAT"]
USER = ["WAVES"] + ["USER%d" % i for i in range(2, 17)]
ME_DISP = ["NOISE " + n for n in NOISE] + ["VPM " + n for n in VPM] + ["USR " + n for n in USER]


def k10(key, name, default=512, **kw):
    """a 10-bit KORG knob 0..1023; the engine supplies the value text (cents, %, Hz...)"""
    p = {"key": key, "name": name, "min": 0, "max": 1023, "default": default, "display": "int", "dynamic_display": True}
    p.update(kw)
    return p


def k7(key, name, default=64, **kw):
    p = {"key": key, "name": name, "min": 0, "max": 127, "default": default, "display": "int", "dynamic_display": True}
    p.update(kw)
    return p


def opt(key, name, options, default=0, **kw):
    p = {"key": key, "name": name, "options": options, "default": default}
    p.update(kw)
    return p


def btn(key, name):
    return {"key": key, "name": name, "display": "string", "min": 0, "max": 1, "default": 0, "momentary": True}


def text(key, name):
    return {"key": key, "name": name, "display": "string", "min": 0, "max": 1, "default": 0}


def params():
    P = []
    # ---- page 1: MAIN (effects, display, program, L.F. COMP)
    P += [opt("modfx_sw", "MOD EFFECT", ["Off", "On", "Select"], 0),
          k10("modfx_speed", "MOD SPEED", 400), k10("modfx_depth", "MOD DEPTH", 500),
          opt("dlrv_sw", "DELAY/REVERB", ["Off", "Delay", "Reverb"], 0),
          k10("dlrv_time", "DL/RV TIME", 500), k10("dlrv_depth", "DL/RV DEPTH", 400),
          {"key": "value", "name": "PROGRAM/VALUE", "min": 0, "max": 1, "default": 0.5, "dynamic_display": True},
          ]
    P += [btn("pgm%d" % (i + 1), "PAGE %d" % (i + 1)) for i in range(8)]
    P += [btn("edit", "EDIT MODE"), btn("write", "WRITE"), btn("exit", "EXIT"),
          opt("shift", "SHIFT", ["Off", "On"], 0),
          btn("oct_dn", "OCTAVE DOWN"), btn("oct_up", "OCTAVE UP"),
          opt("lfc_sw", "L.F. COMP", ["Off", "On"], 0),
          {"key": "lfc_gain", "name": "COMP GAIN", "min": 0, "max": 1, "default": 0.3, "dynamic_display": True},
          {"key": "lfc_vu", "name": "COMP METER", "min": 0, "max": 1, "default": 0.708},
          text("oled1", "DISPLAY 1"), text("oled2", "DISPLAY 2")]
    # ---- page 2: VOICE (wheels, master, timbre, voice mode, arpeggiator)
    P += [{"key": "bend", "name": "PITCH BEND", "min": -1, "max": 1, "default": 0, "dynamic_display": True},
          {"key": "modwheel", "name": "MOD WHEEL", "min": 0, "max": 127, "default": 0, "display": "int"},
          {"key": "master", "name": "MASTER", "min": 0, "max": 1, "default": 0.7, "dynamic_display": True},
          k7("balance", "MAIN/SUB BAL", 64),
          opt("timbre_type", "TIMBRE TYPE", ["Layer", "X-Fade", "Split"], 0),
          opt("edit_timbre", "PANEL", ["Main", "Main+Sub", "Sub"], 0),
          opt("sub_on", "SUB ON", ["Off", "On"], 0),
          {"key": "portamento", "name": "PORTAMENTO", "min": 0, "max": 128, "default": 0, "display": "int",
           "dynamic_display": True},
          k7("voice_spread", "VOICE SPREAD", 0),
          k10("vm_depth", "VOICE MODE DEPTH", 0),
          btn("vm_select", "VOICE MODE SELECT"),
          opt("vm_type", "VOICE MODE", ["Poly", "Mono", "Unison", "Chord"], 0),
          btn("arp_btn", "ARP ON/LATCH"), btn("arp_tempo_btn", "ARP TEMPO"), btn("arp_range_btn", "ARP RANGE"),
          btn("arp_type_btn", "ARP TYPE"),
          opt("arp_mode", "ARPEGGIATOR", ["Off", "On", "Latch"], 0),
          opt("arp_range", "ARP RANGE", ["1 Oct", "2 Oct", "3 Oct", "4 Oct"], 0),
          opt("arp_type", "ARP TYPE", ["Manual", "Rise", "Fall", "Rise Fall", "Random", "Poly Random"], 1),
          {"key": "tempo", "name": "TEMPO", "min": 30, "max": 600, "default": 120, "display": "int"}]
    # ---- page 3: OSC (VCO1, VCO2, multi engine, mixer)
    P += [opt("vco1_wave", "VCO1 WAVE", WAVE, 2), opt("vco1_oct", "VCO1 OCTAVE", OCT, 2),
          k10("vco1_pitch", "VCO1 PITCH", 512), k10("vco1_shape", "VCO1 SHAPE", 0),
          opt("peg_target", "PITCH EG", ["VCO2", "VCO1+2", "All"], 0), k10("peg_int", "PITCH EG INT", 512),
          opt("vco2_wave", "VCO2 WAVE", WAVE, 2), opt("vco2_oct", "VCO2 OCTAVE", OCT, 2),
          k10("vco2_pitch", "VCO2 PITCH", 512), k10("vco2_shape", "VCO2 SHAPE", 0),
          opt("sync_ring", "SYNC/RING", ["Ring", "Off", "Sync"], 1), k10("cross_mod", "CROSS MOD", 0),
          opt("multi_type", "MULTI ENGINE", ["Noise", "VPM", "User"], 0), opt("multi_oct", "MULTI OCTAVE", OCT, 2),
          {"key": "multi_sel", "name": "MULTI TYPE", "min": 0, "max": 15, "default": 0, "display": "int",
           "dynamic_display": True, "qlink_ticks": 3},
          k10("multi_shape", "MULTI SHAPE", 0),
          k10("vco1_level", "VCO1 LEVEL", 1023), k10("vco2_level", "VCO2 LEVEL", 0), k10("multi_level", "MULTI LEVEL", 0),
          opt("me_disp", "MULTI DISPLAY", ME_DISP, 0)]
    # ---- page 4: FILTER, EGs, LFO
    P += [k10("cutoff", "CUTOFF", 1023), k10("resonance", "RESONANCE", 0), k10("cutoff_eg", "CUTOFF EG INT", 512),
          opt("drive", "DRIVE", ["0%", "50%", "100%"], 0), opt("low_cut", "LOW CUT", ["Off", "On"], 0),
          opt("keytrack", "KEYTRACK", ["0%", "50%", "100%"], 0),
          k10("amp_a", "AMP ATTACK", 0), k10("amp_d", "AMP DECAY", 512), k10("amp_s", "AMP SUSTAIN", 1023),
          k10("amp_r", "AMP RELEASE", 300),
          k10("eg_a", "EG ATTACK", 0), k10("eg_d", "EG DECAY", 512), k10("eg_s", "EG SUSTAIN", 0),
          k10("eg_r", "EG RELEASE", 300),
          opt("lfo_wave", "LFO WAVE", WAVE, 1), opt("lfo_mode", "LFO MODE", ["BPM", "Slow", "Fast"], 1),
          k10("lfo_rate", "LFO RATE", 512),
          {"key": "lfo_int", "name": "LFO INT", "min": -511, "max": 511, "default": 0, "display": "int",
           "dynamic_display": True},
          opt("lfo_target", "LFO TARGET", ["Cutoff", "Shape", "Pitch"], 2)]
    # ---- hidden: the engine's program list (MPC's PRESET menu)
    P += [{"key": "program", "name": "PROGRAM", "min": 0, "max": 499, "default": 0, "display": "int",
           "dynamic_display": True, "nudge_pct": 10}]
    return P


def main():
    ps = params()
    json.dump({"params": ps}, open(os.path.join(ROOT, "params.json"), "w"), indent=1)
    lines = ["/* GENERATED by tools/gen_params.py: do not edit. The plugin's parameters, in VST index order. */",
             "#pragma once", "enum {"]
    for i, p in enumerate(ps):
        lines.append("    P_%s = %d," % (p["key"].upper(), i))
    lines += ["    P_COUNT = %d" % len(ps), "};",
              "static const char *const PARAM_KEYS[P_COUNT] = {" + ", ".join('"%s"' % p["key"] for p in ps) + "};"]
    mins = ", ".join(str(p.get("min", 0)) for p in ps)
    maxs = ", ".join(str(len(p["options"]) - 1 if "options" in p else p.get("max", 1)) for p in ps)
    lines += ["static const float PARAM_MIN[P_COUNT] = {%s};" % mins, "static const float PARAM_MAX[P_COUNT] = {%s};" % maxs]
    lines += ["static const char *const ME_DISP_NAMES[%d] = {%s};" % (len(ME_DISP), ", ".join('"%s"' % n for n in ME_DISP)),
              "static const char *const VPM_NAMES[16] = {%s};" % ", ".join('"%s"' % n for n in VPM),
              "static const char *const NOISE_NAMES[4] = {%s};" % ", ".join('"%s"' % n for n in NOISE),
              "static const char *const USER_NAMES[16] = {%s};" % ", ".join('"%s"' % n for n in USER)]
    open(os.path.join(ROOT, "src/param_ids.h"), "w").write("\n".join(lines) + "\n")
    print("params: %d" % len(ps))


if __name__ == "__main__":
    main()
