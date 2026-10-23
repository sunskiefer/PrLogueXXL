# PrLogueXXL

A KORG prologue-16 style polyphonic synthesizer for the Akai Force and MPC (MPC OS VST2, built on mpc-vst-plugins),
by L'Cronx.

- **16 voices** (8 + 8 with the Sub timbre on), the prologue's signal chain: VCO1 and VCO2 (SQR / TRI / SAW with SHAPE,
  octave, pitch, pitch EG, SYNC / RING, CROSS MOD), the MULTI ENGINE (NOISE High / Low / Peak / Decim, VPM with its
  16 types and 6 parameters, USER), mixer, two-pole VCF with DRIVE, LOW CUT and KEYTRACK, AMP EG, EG, LFO
  (SQR / TRI / SAW, BPM / SLOW / FAST, Cutoff / Shape / Pitch).
- **Voice modes**: POLY (with DUO), MONO (sub voices one and two octaves down), UNISON (0-50 cent detune), CHORD
  (the 14 chords).
- **Timbres**: Main and Sub, Layer / X-Fade / Split, balance; the panel edits Main, Main+Sub or Sub.
- **Effects**: MOD FX (Chorus 8, Ensemble 3, Phaser 8, Flanger 8 types), DELAY (12 types incl. BPM) and REVERB
  (10 types), routable per timbre, then the **L.F. COMP** with its working meter (it shows the compression).
- **Arpeggiator**: Manual, Rise, Fall, Rise Fall, Random, Poly Random; 1-4 octaves; latch; runs on MPC's grid while
  MPC plays.
- **USER oscillator slot 1**: KORG's own "waves" oscillator from the logue SDK, with KORG's wavetables A-F.
- **Programs**: 500 slots, 22 factory programs, PROGRAM EDIT pages 1-8 and GLOBAL EDIT (SHIFT + EDIT MODE) as on the
  prologue; WRITE stores to `/sdcard/PrLogueXXL/programs.txt`. MIDI: notes, velocity, pitch bend, mod wheel,
  aftertouch, damper, program change + bank select, and KORG's prologue CC chart.

The panel is four pages made from photos of the prologue (collage): MAIN (effects, OLED, PROGRAM/VALUE, program and
edit buttons, octave, L.F. COMP), VOICE (wheels, master, timbre, portamento, voice spread, voice mode, arpeggiator),
OSC (VCO1, VCO2, multi engine with its 7-segment display, mixer), FILTER (VCF, AMP EG, EG, LFO).

See `docs/MANUAL.md` for how to use it and `docs/PROLOGUE_SPEC.md` for where every value comes from (KORG's documents;
choices KORG does not document are marked as PrLogueXXL's own).

## Build

    ./build.sh                     # -> build/package/L'Cronx - VST - PrLogueXXL/
    test/run_tests.sh              # offline tests (x86 ASan, framework host test, armv7 under qemu)

Needs python3 with Pillow and numpy, Playwright's Chromium (the skin renderer) and Zig (`pip install ziglang`).

## Licence

PrLogueXXL is GPL-3.0-or-later (`LICENSE`); third-party parts keep their own licences (below).

## Credits and licences

KORG logue SDK headers, the "waves" oscillator and its wavetables (BSD-3-Clause, `third_party/logue-sdk`).
mpc-vst-plugins wrapper and tools. Barlow Semi Condensed and Titillium Web fonts (OFL). The panel art is made from
photos of the prologue. "prologue" is a trademark of KORG Inc.; this is an independent project, not affiliated with
KORG.
