# PrLogueXXL: prologue reference (from KORG documents only)

Sources: KORG prologue MIDI Implementation Rev 1.01 (2020-02-10, `prologue_MIDIImp.txt`) [MI], prologue Owner's
Manual [OM], KORG logue-sdk `platform/prologue` [SDK]. Values not in these documents are marked **OWN** where the
engine has to choose something; nothing else is invented.

## Program parameters (MI TABLE 3, timbre block +offset)
10-bit values are 0..1023. Enumerations in KORG's order:

| Parameter | Range / values |
|---|---|
| VOICE MODE TYPE (P14) | 0 POLY, 1 MONO, 2 UNISON, 3 CHORD |
| VOICE MODE DEPTH (P13) | POLY: 0–255 Poly, 256–1023 Duo 0–1023 · MONO: Sub 0–1023 · UNISON: Detune 0–50 cent · CHORD: 14 bands of ~73: 5th, sus2, m, Maj, sus4, m7, 7, 7sus4, Maj7, aug, dim, m7b5, mMaj7, Maj7b5 (0–73, 74–146, 147–219, 220–292, 293–365, 366–438, 439–511, 512–585, 586–658, 659–731, 732–804, 805–877, 878–950, 951–1023) |
| VCO1/2 WAVE, LFO WAVE (P15) | 0 SQR, 1 TRI, 2 SAW |
| VCO1/2 OCTAVE, MULTI OCTAVE | 0–3 = 2', 4', 8', 16' |
| VCO1/2 PITCH (P16) | 0–4: −1200 · 4–356: −1200…−256 · 356–476: −256…−16 · 476–492: −16…0 · 492–532: 0 · 532–548: 0…16 · 548–668: 16…256 · 668–1020: 256…1200 · 1020–1023: 1200 cent (piecewise linear) |
| PITCH EG TARGET | 0–2, printed "VCO1, +, VCO2"; panel/OM: VCO2, VCO1+2, ALL (ALL includes the multi engine) |
| PITCH EG INT (P17) | same breakpoints as P16 with ±64 / ±1024 / ±4800 cent |
| RING/SYNC | 0 RING ON, 1 OFF, 2 SYNC ON (ring: VCO1 × VCO2; sync: VCO2 synced to VCO1) |
| CROSS MOD DEPTH | 0–1023, VCO1 modulates VCO2 pitch |
| MULTI ROUTING | 0 Pre VCF, 1 Post VCF |
| MULTI TYPE | 0 NOISE, 1 VPM, 2 USER |
| SELECT NOISE (P18) | 0 HIGH, 1 LOW, 2 PEAK, 3 DECIM |
| SELECT VPM (P19) | SIN1 SIN2 SIN3 SIN4 SAW1 SAW2 SQU1 SQU2 FAT1 FAT2 AIR1 AIR2 DECAY1 DECAY2 CREEP THROAT |
| SHAPE NOISE / VPM / USER, SHIFT SHAPE VPM / USER | 0–1023 each (stored separately per engine) |
| VCO1/VCO2/MULTI LEVEL, CUTOFF, RESONANCE | 0–1023 |
| CUTOFF EG INT (P20) | 0–11: −100 % · 11–492: −((492−v)²·4641·100)/2³⁰ % · 492–532: 0 · 532–1013: ((v−532)²·4641·100)/2³⁰ % · 1013–1023: +100 % |
| CUTOFF DRIVE, KEYBOARD TRACK (P21) | 0 0 %, 1 50 %, 2 100 % |
| LOW CUT | off/on |
| CUTOFF VELOCITY (OM "EG Velocity"), AMP VELOCITY | 0–127 |
| AMP EG / EG A D S R | 0–1023 each |
| LFO MODE | 0 BPM, 1 SLOW (0.05–28 Hz, OM), 2 FAST (0.5 Hz–2.8 kHz, OM) |
| LFO RATE (P22) | BPM: 16 bands of 64 = 4, 2, 1, 3/4, 1/2, 3/8, 1/3, 1/4, 3/16, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/36 |
| LFO INT | 0–1023 (OM: 0..511, SHIFT for 0..−511) |
| LFO TARGET | 0 CUTOFF, 1 SHAPE, 2 PITCH |
| LFO TARGET OSC (P27) | 0 ALL, 1 VCO1+VCO2, 2 VCO2, 3 MULTI |
| LFO KEY SYNC, LFO VOICE SYNC, MONO LEGATO (OM "EG Legato") | off/on |
| MOD WHEEL ASSIGN (P23, stored 2–32 per table) / AFTER TOUCH | BALANCE, PORTAMENTO, V.SPREAD, V.M DEPTH, VCO1 PITCH, VCO1 SHAPE, VCO2 PITCH, VCO2 SHAPE, CROSS MOD, PITCH EG INT, MULTI SHAPE, VCO1 LEVEL, VCO2 LEVEL, MULTI LEVEL, CUTOFF, RESONANCE, CUTOFF EG INT, A.EG A/D/S/R, EG A/D/S/R, LFO RATE, LFO INT, MOD FX SPEED, MOD FX DEPTH, DL/RV TIME, DL/RV DEPTH, GATE TIME |
| MOD WHEEL RANGE | −100…+100 % |
| E.PEDAL ASSIGN (P24) | OFF, VOLUME, then the same list as P23 up to DL/RV DEPTH |
| BEND RANGE +/− | OFF, 1–12 notes |
| VPM PARAM 1–6 | −100…+100 %: Feedback, Noise Depth, Shape Mod Int, Mod Attack, Mod Decay, Mod KeyTrack (0 % = the type's own value) |
| USER PARAM 1–6 (P25/P26) | type 0 Percent 0–100 %, 1 Bipolar −100…100, 2 Select 1–101 |
| PORTAMENTO TIME / VOICE SPREAD | 0–127 (OM: portamento OFF, 0…127) |

Common: OCTAVE −2…+2, TIMBRE TYPE Layer/XFade/Split, MAIN/SUB BALANCE 0–127, POSITION, SPLIT POINT, TEMPO
30.0–600.0, ARP TARGET, AMP VELOCITY, PORTAMENTO MODE Auto/On, PROGRAM LEVEL −18…+6 dB, MOD FX type
(CHORUS, ENSEMBLE, PHASER, FLANGER, USER) with subtypes, MICRO TUNING (P8), SCALE KEY, PROGRAM TUNING ±50 cent,
TRANSPOSE ±12, ARP GATE TIME 0–100 %, ARP RATE (P9: 64th, 48th, 32th, 24th, 16th, 16.t, 12th, 8th, 8.th, 6th, 4th),
DELAY/REVERB TYPE OFF/DELAY/REVERB, TIME, DEPTH, DRY WET, REVERB TYPE (P10: HALL, SMOOTH, ARENA, PLATE, ROOM,
EARLY REF, SPACE, RISER, SUBMARINE, HORROR, USER1–8), DELAY TYPE (P11: STEREO, MONO, PING PONG, HIPASS, TAPE,
ONE TAP, STEREO BPM, MONO BPM, PING BPM, HIPASS BPM, TAPE BPM, DOUBLING, USER1–8), FX routing, ARPEGGIATOR
OFF/ON/LATCH, RANGE, TYPE (P12: MANUAL, RISE, FALL, RISE FALL, RANDOM, POLY RANDOM).

## Not in KORG's documents (engine must choose: OWN)
Envelope times in seconds, velocity curve tables, Duo/Sub level curves, VPM ratio tables per type, filter slope
(a dealer page quotes 2-pole 12 dB/oct; not a KORG document), noise SHIFT+SHAPE, L.F. COMP curve and what its VU
shows, VCO SHAPE behaviour for SAW/TRI, XFADE range, ARP RANGE byte → octaves.
