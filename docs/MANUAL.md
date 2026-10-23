# PrLogueXXL manual

## Install
Unzip the release on a computer, copy it to the Force/MPC and run `install.sh` (see the release's INSTALL.md), or copy
the folder `L'Cronx - VST - PrLogueXXL` into `/sdcard/Synths/` and add the plugin entry as INSTALL.md shows. Your
programs live in `/sdcard/PrLogueXXL/programs.txt`.

## The four pages
Swipe between MAIN, VOICE, OSC and FILTER. Knobs turn by dragging up/down; lever switches move by dragging up/down
(or a Q-Link); buttons are tapped. Each page's 16 Q-Links follow its controls. The OLED on MAIN always shows the last
thing you touched (name and value); after two seconds it goes back to the program.

### MAIN
- **MOD EFFECT**: lever OFF / ON / SELECT. Push it to SELECT to step the type (Chorus, Ensemble, Phaser, Flanger); it
  springs back to ON, as on the prologue. SHIFT + SELECT steps the subtype. For three seconds after, the
  PROGRAM/VALUE knob picks the subtype. SPEED, DEPTH.
- **DELAY / REVERB**: lever OFF / DELAY / REVERB; moving it shows the type and the PROGRAM/VALUE knob picks the subtype
  for three seconds. TIME, DEPTH (dry/wet is in PROGRAM EDIT page 8).
- **PROGRAM / VALUE**: an endless knob. Outside the menus it selects the program; in the menus it changes the value
  shown; after TEMPO it sets the tempo.
- **PROGRAM SORT / EDIT PAGE 1-8**: outside the menus, 8 favourite programs (SHIFT + button stores the current one).
  In PROGRAM EDIT they open the pages; pressing the same button again goes to the page's next item.
- **EDIT MODE** opens PROGRAM EDIT (SHIFT + EDIT MODE: GLOBAL EDIT). **WRITE** stores the program in its slot (the
  button lights while the program has unsaved edits). **EXIT** leaves the menus. **SHIFT** latches (tap it on, tap
  it off): SHIFT + MULTI SHAPE = SHIFT SHAPE, SHIFT + SELECT = subtype, SHIFT + a page button = store favourite.
- **OCTAVE** down / up (-2..+2).
- **L.F. COMP**: ON / OFF, GAIN (turning it right lowers the threshold and raises the gain). The meter shows how much
  the compressor works: it rests at 0 and swings left as it compresses. MASTER (page VOICE) drives it too.

### VOICE
PITCH and MOD wheels (pitch bend range and mod wheel destination: PROGRAM EDIT page 4), MASTER, TIMBRE (MAIN/SUB
BALANCE, TYPE Layer / X-Fade / Split, PANEL = which timbre the panel edits: Main, Main+Sub, Sub, with its LEDs; SUB
ON), PORTAMENTO, VOICE SPREAD, VOICE MODE DEPTH, VOICE MODE SELECT (Poly, Mono, Unison, Chord; the LEDs show it),
ARPEGGIATOR: ON/LATCH (Off, On, Latch), TEMPO (then the PROGRAM/VALUE knob; while MPC plays, MPC's tempo is used),
RANGE (1-4 octaves), TYPE.

VOICE MODE DEPTH: Poly = Poly, then Duo (2 voices per note, the second's level and detune rise); Mono = Sub (voices
one octave down, then two octaves down); Unison = detune 0-50 cent; Chord = 5th, sus2, m, Maj, sus4, m7, 7, 7sus4,
Maj7, aug, dim, m7b5, mMaj7, Maj7b5.

### OSC
VCO1: WAVE, OCTAVE (16' 8' 4' 2', LEDs), PITCH (+-1200 cent), SHAPE; PITCH EG target (VCO2, VCO1+2, All) and INT
(+-4800 cent). VCO2: the same, SYNC / OFF / RING, CROSS MOD DEPTH. MULTI ENGINE: ENGINE (USR, VPM, NOISE), OCTAVE,
the red display shows the type, TYPE (endless), SHAPE (SHIFT: SHIFT SHAPE = VPM ratio). MIXER: VCO1, VCO2, MULTI.

### FILTER
VCF: CUTOFF, RESONANCE, EG INT (+-, KORG's curve), DRIVE 0/50/100, LOW CUT, KEYTRACK 0/50/100. AMP EG and EG:
ATTACK, DECAY, SUSTAIN, RELEASE. LFO: WAVE, MODE (Fast, Slow, BPM), RATE, INT (+-), TARGET (Pitch, Shape, Cutoff).

## PROGRAM EDIT (EDIT MODE)
1 SORT: category. 2 NAME: 12 characters (button 2 again = next character, the knob picks the letter). 3 TIMBRE: sub
timbre, edit timbre, type, balance, position, split point. 4 PEDAL & WHEEL: mod wheel assign and range, bend range +/-,
aftertouch assign. 5 MODULATION: LFO target OSC, LFO key sync, LFO voice sync, EG velocity, amp velocity.
6 OTHER: arp target, arp rate, arp gate time, multi routing (pre/post VCF), EG legato, portamento mode, voice spread,
program level, program tuning, transpose, micro tuning, scale key, tempo. 7 MULTI ENGINE: the VPM parameters
(Feedback, Noise Depth, Shape Mod Int, Mod Attack, Mod Decay, Mod KeyTrack) or the USER oscillator's parameters.
8 EFFECTS: mod fx type, subtype, routing; delay/reverb type, subtype, dry/wet, routing.

GLOBAL EDIT (SHIFT + EDIT MODE): 1 master tune, transpose; 2 velocity curve (Type 1-8, Const127).

## MIDI
Notes and velocity, pitch bend, CC1 mod wheel, CC64 damper, channel aftertouch (its destination: page 4), program
change with bank select LSB 0-4 (programs 1-100, 101-200 ...), CC120 / 123, and KORG's prologue CC chart (cutoff
CC43, resonance CC44, ... docs/PROLOGUE_SPEC.md).

## What is KORG's and what is PrLogueXXL's own
Ranges, curves, types and their order follow KORG's MIDI implementation and owner's manual. KORG does not publish the
analog circuits, the envelope times, the VPM ratio tables, the velocity curves or the L.F. COMP's settings: those are
PrLogueXXL's own models, from the information collected in docs/PROLOGUE_SPEC.md and docs/COMMUNITY_RESEARCH.md.
USER slots 2-16 are empty; prologue user units (.prlgunit, Cortex-M4 code) are not loaded.
