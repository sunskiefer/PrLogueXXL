# PrLogueXXL: community and secondary research on gaps in KORG's prologue documents

Compiled 2026-10-23. This fills in the gaps listed in `PROLOGUE_SPEC.md` ("Not in KORG's documents").
No values here are estimates. A gap with no evidence is marked **NOT FOUND**.

## Confidence labels
- **KORG official**: KORG's own documents (product page, owner's manuals, logue-sdk source).
- **KORG staff statement**: a named KORG engineer quoted in an interview or forum.
- **Named reviewer/user observation**: a measurement or listening observation by a named person, such as a magazine reviewer. This is the closest match to "measured by a named user" that was found. No oscilloscope or DAW measurements with numbers were found.
- **Hearsay**: anonymous or unsourced.

## Access limits during this research
These sources could not be read from this environment, so their contents are **unchecked**, not "empty":
- Gearspace "Korg Prologue" mega-thread (`gearspace.com/threads/korg-prologue.1197859/…`): client error.
- KVR threads, including "Korg Prologue – anybody dived into vpm oscillators…" (https://www.kvraudio.com/forum/viewtopic.php?t=532302) and "Korg ProLogue opinions": client error.
- ModWiggler thread "minilogue 'shape' on saw-core vco" (https://modwiggler.com/forum/viewtopic.php?t=173372): client error. Its title suggests it covers item 3 directly. It is the best lead left to read by hand.
- Reddit and Elektronauts search endpoints: blocked (no permission).
- YouTube transcripts: none retrieved.
- korgforums.com: no relevant hits from search.

## Primary documents used
- **[OM]** prologue Owner's Manual (KORG CDN): https://cdn.korg.com/us/support/download/files/9317b94261e53e700b0145502850edc2.pdf
- **[PP]** KORG prologue product page (AU mirror; the US and UK pages failed to load): https://www.korg.com/au/products/synthesizers/prologue
- **[SPEC]** KORG prologue specifications: https://korg.com/us/products/synthesizers/prologue/specifications.php
- **[SDKPAGE]** KORG prologue SDK page: https://www.korg.com/us/products/synthesizers/prologue/sdk.php
- **[XDOM]** minilogue xd / xd module Owner's Manual E3 (KORG CDN): https://cdn.korg.com/us/support/download/files/efcc587e39602e53d3ebe8d4bc6c8499.pdf?filename=minilogue_xd_xdMod_OM_E3.pdf
- **[MNLGOM]** original minilogue Owner's Manual E3 (KORG document, library mirror): https://aadl.org/files/catalog_guides/minilogue_OM_E3.pdf
- **[SDK]** korginc/logue-sdk @ e78b16c6068b1c30fc072f2aebb8da5eef6c005c (2026-10-20), `platform/prologue/`: https://github.com/korginc/logue-sdk/tree/master/platform/prologue

---

## 1. Envelope times (AMP EG / EG, 0–1023)

| Finding | Source | Confidence |
|---|---|---|
| OM defines attack, decay and release only as "the time required…". No seconds, no curve. | [OM] | KORG official |
| "the longest attack time is just three seconds". | Gordon Reid, *Sound On Sound*, Apr 2018 prologue review: https://www.soundonsound.com/reviews/korg-prologue (fetched as https://www.soundonsound.com/node/4918545) | Named reviewer observation |
| Curve: the contours' phases "seem to be rather linear, which can result in a cusp as the decay stage transitions into the sustain stage." | same SOS review | Named reviewer observation (by ear, no measurement) |
| minilogue xd: "at just three seconds, the slowest attack times…" are too fast for the reviewer's taste. The xd contours are digital. | Gordon Reid, *SOS*, Jun 2019 minilogue xd review: https://www.soundonsound.com/node/4920265 | Named reviewer observation |
| xd OM gives no times or curve. | [XDOM] | KORG official |

- Decay and release maximum times in seconds: **NOT FOUND**.
- The 0–1023 → seconds mapping: **NOT FOUND**.
- Attack curve shape other than "rather linear": **NOT FOUND**.

boochow's prologue↔xd converter copies AMP EG values 1:1 between the two synths (https://github.com/boochow/prologue_tools, `prlg2mnlgxd.py`). This shows only the converter author's assumption that the scales match. It is not evidence of seconds.

## 2. Filter

| Finding | Source | Confidence |
|---|---|---|
| "a two-pole low pass filter with enhanced sharpness". | [PP] | **KORG official** (product page, not dealer copy) |
| "drive that adds powerful distortion, and a low cut (high pass) switch". | [PP] | KORG official |
| DRIVE switch [0%, 50%, 100%]: "Sets the distortion effect produced by the drive circuit, in three stages". "Drive circuit" means the drive is part of the analog voice path. | [OM] | KORG official |
| LOW CUT [OFF, ON]: "Trims the low range of the sound via high-pass filter." | [OM] | KORG official |
| "The VCF of the Prologue, however, is completely new. This new VCF is wider and brighter than the Minilogue's." | Yoshihito Yamada (KORG), Attack Magazine interview: https://www.attackmagazine.com/features/interview/analogue-plus-digital-designing-korg-prologue/ | **KORG staff statement** |
| "a resonant 12dB/octave low-pass device…". No 24 dB mode. | Gordon Reid, SOS prologue review | Named reviewer (12 dB/oct follows from KORG's "two-pole") |
| Self-oscillation: "The Prologue's low-pass filters (one per voice) will oscillate when the resonance is high enough" and can be played chromatically. | same SOS review | Named reviewer observation |
| "a gentle high-pass filter controlled merely by an on/off switch". | same SOS review | Named reviewer observation |
| "the filter is a fixed 2-pole design…". "I found the resonance to be quite quick in feeding back." | Dave Gale, MusicTech, 2018-06-18: https://musictech.com/2018/06/korg-prologue-review/ | Named reviewer |
| minilogue xd filter: Reid "suspect[s] is a 12dB/oct design similar to the Prologue's". KORG does not state the xd's slope. | SOS xd review | Named reviewer (guess) |
| For contrast, the original minilogue has a 2-POLE/4-POLE switch: 12 dB/oct and 24 dB/oct. | [MNLGOM] | KORG official (minilogue only, a different VCF) |

- Filter topology (circuit type, e.g. OTA or SVF): **NOT FOUND**. KORG says only that it is "completely new" and not the minilogue's VCF.
- Low-cut corner frequency and slope: **NOT FOUND**.
- Drive transfer curve and gain amounts: **NOT FOUND**.
- Schematics: none public for the prologue. Circuit diagrams exist only for the original minilogue (https://www.servicemanual.altervista.org/KORG_Service-Manual-Schematics.html lists "KORG Minilogue … Circuit diagrams", 17 pages). The prologue VCF is a different design.

## 3. VCOs: SHAPE, range, cross mod, ring and sync

| Finding | Source | Confidence |
|---|---|---|
| SHAPE "will determine the final shape, complexity, or duty-cycle (Square) of the selected waveform". There is no per-wave description. | [OM], same wording in [XDOM] and [MNLGOM] | KORG official |
| Panel: "wave shaping to shape the oscillator's overtones and a sync/ring switch". | [PP] | KORG official |
| "the oscillators are based on the Minilogue, we brushed up the implementation." | Yoshihito Yamada (KORG), Attack Magazine interview (link above) | **KORG staff statement** |
| OCTAVE 2', 4', 8', 16'. PITCH ±1200 cent in 1-cent steps. | [OM] | KORG official |
| Cross Mod: "allows Oscillator 1 to modulate the pitch of Oscillator 2". Ring: VCO1 ring-modulates VCO2. Sync: VCO2's phase is locked to VCO1's. | [OM] | KORG official |
| minilogue (same VCO lineage) Ring "outputs the sum and difference of the frequencies created by the two oscillators". | [MNLGOM] | KORG official (minilogue) |
| minilogue/monologue SAW + SHAPE: "morphs the waveform closer to a square-like shape… gradually attenuates the sawtooth's even-numbered harmonics, emphasizing the odd partials." | MusicRadar "How to master the Korg Minilogue and Korg Monologue": https://www.musicradar.com/how-to/how-to-master-the-korg-minilogue-and-korg-monologue | Named publication, by ear (minilogue, not the prologue) |
| minilogue/monologue TRI + SHAPE: "turning the Shape knob to maximum adds higher-frequency content while strongly emphasizing the third and fifth harmonics." | same MusicRadar | same |
| minilogue/monologue SQR + SHAPE: pulse-width adjustment. | same MusicRadar ("the pulse-width adjustment that we know and love"); [OM]/[MNLGOM] "duty-cycle (Square)" | KORG official (duty cycle) + named publication |
| SAW octave anomaly: "On the original Minilogue, the sawtooth wave sat an octave above the square and triangle waves unless waveshaping was applied, which dropped it to their octave. On the XD, it sits at the same octave unless you apply waveshaping, at which point it's an octave lower." | Gordon Reid, SOS minilogue xd review (link above) | Named reviewer observation |
| The xd "no longer has the discontinuity in sync sweeps that the original Minilogue had". Reid credits this to a change in the contour architecture, not the oscillators. | same SOS xd review | Named reviewer observation |
| "the voice structures of the XD and the Prologue appear to be almost identical". | same SOS xd review | Named reviewer opinion |

- Prologue-specific per-wave SHAPE behaviour: **NOT FOUND**. Only minilogue/monologue observations exist, plus KORG's statement that the prologue VCOs are "based on the Minilogue".
- Whether the prologue saw shows Reid's octave behaviour: **NOT FOUND**.
- Whether the prologue VCO circuit is identical to the minilogue xd's: **NOT FOUND**. KORG says only that both descend from the minilogue.
- Cross-mod depth in Hz or cents per unit: **NOT FOUND**.
- Ring implementation (analog multiplier vs. other): **NOT FOUND**.
- Hard vs. soft sync: **NOT FOUND** ([OM] describes "phase locked", which suggests hard reset but does not say so).

## 4. VPM (multi engine)

| Finding | Source | Confidence |
|---|---|---|
| "The VPM (Variable Phase Modulation) / FM oscillator was newly developed for prologue." | [PP] | KORG official |
| SHAPE = Modulator Depth "[1.00...15.00]" (prologue OM, as printed). The xd OM prints MOD DEPTH "[0.00:15.00...]", "range changes depending on TYPE". | [OM], [XDOM] | KORG official (note the 1.00 vs 0.00 lower bound; check against the PDF) |
| SHIFT+SHAPE = Modulator Pitch Ratio "[1:4, 1:2, 1:1, 2:1...] (range changes depending on TYPE)". The xd calls it "RATIO OFFSET". "This adjusts the modulator ratio and adds overtones." | [OM], [XDOM] | KORG official |
| Edit params 1–6 (−100…+100 %): Feedback ("depth of the modulator feedback"), Noise Depth ("depth of the noise modulation"), Shape Mod Int ("depth of the internal EG effect"), Mod Attack / Mod Decay (attack/decay "of the internal EG"), Mod KeyTrack ("depth of the modulator effect via keyboard tracking"). | [XDOM] (the prologue OM has the same parameters) | KORG official |
| Structure: one carrier + one modulator ("2-op FM"). | [OM]/[XDOM] wording quoted by users; SOS xd review "16 variations of 2-op FM synthesis"; MusicTech "a single carrier and modulator" | KORG official + reviewers |

Prologue OM type descriptions (verbatim):
- Sin1: "Modulated sine wave carrier basic type."
- Sin2: "Sine wave carrier and a modulator with self-feedback."
- Sin3: "Sine wave carrier with modulator 3 harmonics higher."
- Sin4: "Sine wave carrier with modulator 5 harmonics higher."
- Saw1: "Modulated sawtooth carrier basic type."
- Saw2: "Sine carrier based pseudo-sawtooth type."
- Squ1: "Modulated square wave carrier basic type."
- Squ2: "Sine carrier based pseudo-square wave type."
- Fat1: "Second subharmonic modulator with self-feedback and driven carrier output."
- Fat2: "Half subharmonic modulator with self-feedback and driven carrier output."
- Air1: "Noise modulated sine wave carrier."
- Air2: "Sine wave carrier modulated by both noise and a sine wave."
- Decay1: "Type with decaying modulation amount."
- Decay2: "Type with strong decaying modulation amount."
- Creep, Throat: "Experimental type with complex and evolving modulations."

The minilogue xd OM words some types differently. Its Fat1 is a "1/4 subharmonic modulator…" and Fat2 a "3/4 subharmonic modulator…", both "…emphasizing the lower harmonics". Its Sin3 and Sin4 have "3x" and "5x harmonics" modulators. The prologue's "Second subharmonic" / "Half subharmonic" and the xd's "1/4" / "3/4" are KORG's own two wordings. This research does not reconcile them.

- Per-type list of SHIFT+SHAPE ratio values (endpoints, steps): **NOT FOUND** in KORG documents or any forum that could be read.
- Per-type depth range: **NOT FOUND** beyond "[1.00…15.00]" / "range changes depending on TYPE".
- Internal-EG times, feedback amount, noise amount: **NOT FOUND**.
- Firmware reverse engineering of the VPM engine: **NOT FOUND**. No public prologue or xd firmware disassembly turned up.
- KORG developer statements on VPM internals: **NOT FOUND**. The Attack Magazine interview mentions only "the extremely flexible third oscillator".

## 5. Noise engine

| Finding | Source | Confidence |
|---|---|---|
| HIGH: "A high-pass filter will be used." LOW: "A low-pass filter will be used." PEAK: "A peak filter (bandpass filter) will be used." DECIM: "A Decimator will be used." | [OM] | KORG official |
| SHAPE: High/Low "Cutoff [10.0Hz...21.0kHz]". Peak "Bandwidth [110Hz...880Hz]". Decim "Sample Rate [240Hz...48.0kHz]". | [OM] (same in [XDOM]: Peak "Adjusts the peak characteristic bandwidth") | KORG official |
| **prologue**: there is no SHIFT+SHAPE for noise. Instead, the multi-engine OCTAVE switch with NOISE selected works like this: "The key tracking effect can be adjusted on an octave-by-octave basis, only when "Decim" is selected with the TYPE knob." | [OM] | KORG official |
| **minilogue xd** (differs): SHIFT+SHAPE on Decim = KEY TRACK [0.0%…100.0%], "Adjusts the depth of the sample rate via keyboard tracking." | [XDOM] | KORG official (xd only) |
| "Experience how digital filtering can produce direct changes in the color of the noise." This means the noise filters are digital. | [PP] | KORG official |

- Noise filter order, slope and Q (for High, Low, Peak): **NOT FOUND**.
- What Peak's "Bandwidth 110–880 Hz" means for the centre frequency: **NOT FOUND**. KORG labels the SHAPE quantity as bandwidth.
- Decimator bit-depth reduction: **NOT FOUND**.
- Mapping of Decim key-track to the 2'/4'/8'/16' positions: **NOT FOUND**.

## 6. Voice modes

| Finding | Source | Confidence |
|---|---|---|
| POLY → DUO: "Turn the knob to the right to switch to DUO mode, which stacks two voices when playing a key". Turning further increases the stacked voice's level and the detune depth. In DUO the synth "operates in unison as a poly synth with up to 8 or 4 voices". | [OM], [PP] | KORG official |
| MONO: turning right "will assign voices 2 and 3 as a sub-oscillator one octave lower". Turning further assigns voice 4 as a sub-oscillator two octaves lower. | [OM] | KORG official |
| UNISON: "All voices (16 voices on the prologue-16, 8 voices on the prologue-8) will sound in unison". Detune 0C…50C. | [OM] | KORG official |
| CHORD list: 5th, sus2, m, Maj, sus4, m7, 7, 7sus4, Maj7, aug, dim, m7♭5, mMaj7, Maj7♭5. | [OM] | KORG official |
| Key priority in MONO is fixed as last-note. | SOS prologue review | Named reviewer observation |
| minilogue (original) POLY depth "plays an inverted form of the played chord" (0–8). This is minilogue only, not a prologue feature. | [MNLGOM] | KORG official (minilogue) |

- Duo detune curve and level curve versus depth: **NOT FOUND**.
- Mono sub-oscillator level curve (how depth 0–1023 crossfades voices 2–3 and 4): **NOT FOUND**.
- What MONO does with voices 5–16 (or 5–8): **NOT FOUND**.
- How unison spreads detune across voices (symmetric? linear?): **NOT FOUND**. How Voice Spread pans unison voices: **NOT FOUND**.
- Chord intervals and voicing per chord name: **NOT FOUND**. KORG lists only the names. No source gives semitone sets, octave placement or inversions.

## 7. L.F. COMP (prologue-16)

| Finding | Source | Confidence |
|---|---|---|
| "Newly developed L.F. COMP. (Low Frequency Compressor) analog effect (prologue-16 only)." "It's a low end booster/compressor using an analog circuit". | [PP] | **KORG official** |
| "The exclusive VU meter is available only on the prologue-16, and indicates the newly developed master effect L.F. COMP." | [PP] | KORG official |
| VU meter: "The VU meter shows about how much the output signal is being compressed." When the needle moves left, the compressor is working harder. When L.F. COMP is on, the meter lights and rests near 0 dB. In effect, the meter shows gain reduction. | [OM] | KORG official |
| GAIN: turning right raises the gain and lowers the compressor threshold. MASTER volume also drives it ("higher master volume engages the compressor more"), so use MASTER and GAIN together. The setting is not saved in programs. It sits last in the output chain. | [OM] (paraphrased by the fetch tool; check exact wording in the PDF) | KORG official |
| "an analogue compressor … that sits after the master volume control and thickens the sound considerably". | SOS prologue review | Named reviewer |
| "the compressor will helpfully both compress and replace some of the low-end frequencies". | MusicTech review | Named reviewer |
| "Analog compressor (prologue-16 only) for extra bass boost", non-programmable. | Theo Bloderer, greatsynthesizers.com, 2018-07-13: https://greatsynthesizers.com/en/review/korg-prologue-the-8-16-voice-pros-synth/ | Named reviewer |

- Threshold, ratio, attack and release values: **NOT FOUND**.
- Boost or sidechain crossover frequency: **NOT FOUND**.
- Circuit or schematic: **NOT FOUND**.
- VU ballistics: **NOT FOUND**.

## 8. Velocity, ARP RANGE, XFADE, portamento

| Finding | Source | Confidence |
|---|---|---|
| Velocity curves: Type 1 "requires you to play strongly…", Type 4 "a typical curve", Type 6 "produces an effect without requiring you to play very strongly", Type 7 "fairly steady effect with little change for medium-strength playing", Type 8 like 7 but flatter, Const 127. | [OM], [XDOM] | KORG official (qualitative only) |
| ARP RANGE: each press cycles 1 → 4 octaves. | [OM] | KORG official |
| XFADE: "As you move up the keyboard, the sub timbre will gradually fade into the main timbre." Position Sub↔Main / Main↔Sub. Split Point [C-1…9]. Balance 0…127, where 64 means equal volume. | [OM] | KORG official |
| PORTAMENTO knob [OFF, 0…127]: "Specifies the length of the portamento". Mode Auto/On. | [OM] | KORG official |
| "(Almost) infinitely long Portamento times". | Theo Bloderer, greatsynthesizers.com | Named reviewer (qualitative) |

- Numeric velocity curve tables (curves 1–8): **NOT FOUND**.
- ARP RANGE stored byte → octave mapping: **NOT FOUND**. The OM gives 1–4 octaves. Neither loguetools (`prologue.py` field `arp_range`, https://github.com/gazzar/loguetools) nor prologue_tools decodes the value; prologue_tools only tests `> 0`.
- XFADE: crossfade law, key range, and how Split Point enters into it: **NOT FOUND**.
- Portamento time in seconds, curve, and constant-time vs. constant-rate: **NOT FOUND**.

## 9. Effects

| Finding | Source | Confidence |
|---|---|---|
| "The high-quality digital effects feature pristine 32-bit floating-point processing". There are two effect units: modulation and delay/reverb. | [PP] | KORG official |
| "The final output comes from a DA conversion for the digital effects… we made all efforts to get a completely clear bypass sound." | Etienne Noreau-Hebert (KORG), Attack Magazine interview | **KORG staff statement** |
| Delay feedback maximum is "a little under unity". | SOS prologue review | Named reviewer observation |
| Type descriptions from the OM are verbatim. Delay: Stereo, Mono, Ping Pong, Hi Pass ("high-pass filter on which the low frequency band fades out"), Tape ("Simulates a tape echo"), One Tap ("without feedback"), the *BPM variants synced to TEMPO, Doubling ("short delay with a doubling effect"). Reverb: Hall, Smooth, Arena, Plate, Room, Early Ref, Space ("unstable"), Riser ("echoes one octave higher"), Submarine ("one octave lower"), Horror ("highly unstable tonal quality"). | [OM] | KORG official |
| Mod FX subtype descriptions (xd OM, the same type set). Chorus: Stereo, Light, Deep, Triphase ("three choruses, each with different LFO phases"), Harmonic, Mono, Feedback, Vibrato. Ensemble: Stereo, Light, Mono. Phaser: Stereo, Fast, Orange ("models a popular analog phaser in an orange box"), Small ("classic New York phaser sound from the 1970s"), Small Reso, Black ("phaser sounds from Denmark"), Formant, Twinkle. Flanger: Stereo, Light, Mono, High Sweep, Mid Sweep… (the list was truncated in the fetch). | [XDOM] | KORG official |
| User-effect interface. MODFX: `_hook_process(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames)` plus `_hook_param`. DELFX/REVFX: `_hook_process(float *xn, uint32_t frames)`, one in-place float buffer. The header doesn't state its channel layout. BPM comes from `fx_get_bpm()`. Rate 48 kHz. | [SDK] `inc/usermodfx.h`, `userdelfx.h`, `userrevfx.h`, `fx_api.h` | KORG official |
| User-effect memory. Modfx: SRAM 0x20017800 (6K) + SDRAM 0xC0400000 (128K). Delfx and revfx: SRAM 0x20019000 (12K) + SDRAM 0xC0420000 (2432K) each. They share one address range, which matches the single delay/reverb slot. | [SDK] `dummy-*/ld/user*.ld` | KORG official |

- Internal algorithms of the factory mod/delay/reverb types: **NOT FOUND**. KORG never published any, and no reverse engineering turned up.

## 10. .prlgunit packaging and running units in an emulator

All of the following is **KORG official** (logue-sdk source at commit e78b16c, `platform/prologue/`).

- **Package**: a zip renamed to `<project>.prlgunit`. Inside is one folder `<project>/` containing `manifest.json` and `payload.bin`. Source: `dummy-osc/Makefile` (`PKGARCH := $(PROJECT).prlgunit`, `MANIFEST := manifest.json`, `PAYLOAD := payload.bin`, `zip -r -m -q`).
- **manifest.json** has a `header` object: `platform` ("prologue"), `module` (osc/modfx/delfx/revfx), `api` ("1.1-0"), `dev_id`, `prg_id`, `version`, `name`, `num_param`, `params` [[name, min, max, unit]…]. Source: `dummy-osc/manifest.json`.
- **payload.bin**: `arm-none-eabi-objcopy -O binary` of an ELF built for **Cortex-M4F**. Flags: `-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Os`, `MCU_MODEL := STM32F401xC`, linked with `-nostartfiles`. The binary is plain, position-dependent Thumb-2 code linked at a fixed address. It is not encrypted in the SDK build.
- **Load addresses** (from the linker scripts):
  - osc: `SRAM org = 0x20000000, len = 32K`
  - modfx: `0x20017800` (6K) + SDRAM `0xC0400000` (128K)
  - delfx and revfx: `0x20019000` (12K) + SDRAM `0xC0420000` (2432K)
- **Binary layout** (`ld/rules.ld`): `.hooks` comes first, then `.init_array`, `.text`, `.rodata`, `.data`, `.bss` (NOLOAD), then `.ARM.extab`/`.exidx`.
- **The payload starts with a packed hook table** (`tpl/_unit.c`, `inc/userosc.h`):
  - magic `'UOSC'` / `'UMOD'` / `'UDEL'` / `'UREV'`
  - `uint32 api`, `uint8 platform`, 7 reserved bytes
  - function pointers. For an oscillator: entry, cycle, on, off, mute, value, param, then 5 reserved.
- **Start-up**: `_entry(platform, api)` zeroes `.bss`, runs `.init_array`, then calls `_hook_init`.
- **Oscillator call** (`inc/userosc.h`): `_hook_cycle(const user_osc_param_t*, int32_t *yn, uint32_t frames)` writes Q31 output at 48 kHz (`k_samplerate 48000`). The `user_osc_param_t` struct holds `shape_lfo` (int32), `pitch` (high byte note, low byte fine), `cutoff`, `resonance` (0x0000–0x1fff).
- **Firmware dependency.** A unit calls into the firmware at absolute addresses resolved with `--just-symbols=ld/osc_api.syms` (or `main_api.syms` for effects).
  - Oscillator symbols: `k_osc_api_version = 0x0800f000`, `midi_to_hz_lut_f = 0x0800f100`, `wavesA…F` at `0x080131e8…`, `_osc_white = 0x0801edb8`, `_osc_rand = 0x0801ed80`, `_osc_mcu_hash = 0x0801eabc`, among others.
  - Effect symbols: `0x0807b000…`, including `_fx_get_bpm = 0x0807ca88`.
  - **An emulator must place these lookup tables and functions at exactly these addresses.**
  - `logue-sdk/websim/dsp/` holds KORG-written C copies of the same tables and functions (`osc_api.cpp`, `fx_api.cpp`, `waves-a…f.c`, `midi_to_hz_lut.c`, …; author Etienne Noreau-Hebert, KORG). That simulator targets the NTS-1 mkII and NTS-3. Whether its tables are bit-identical to the prologue's flash contents is **NOT FOUND**.
- **On-device header**: `inc/userprg.h` defines a 1 KB `user_prg_header_t` with fields target, api, dev_id, prg_id, version, name, num_param, params[6], pad and load_size. It is followed by a 0x84-byte signature area ("can fit ECDSA sig using secp521r1"). That header is **not** in payload.bin; the Librarian or logue-cli builds it from manifest.json when loading. How the signature is generated or checked: **NOT FOUND**.
- **Compatibility**: `USER_TARGET_PLATFORM_IS_COMPAT` accepts prologue, minilogue xd and NTS-1 (v1 API 1.1-0). Prologue firmware ≥ 2.00 is needed for SDK 1.1-0 units (`platform/prologue/README.md`).
- **Example**: peterall/eurorack-prologue (https://github.com/peterall/eurorack-prologue) rebuilds units from source per platform. It is not an example of running prebuilt binaries outside the hardware.
- Memory limits, from [SDKPAGE]: "For oscillators, both code and variables must fit in 32KB of RAM"; effects get "additional RAM for delay lines, up to 2MB".

**Conclusion for a loader (from the facts above).** A `.prlgunit` is zip → manifest.json + raw Cortex-M4F image. A loader needs three things:
- a Thumb-2 + FPv4-SP CPU emulator;
- memory at the linked address;
- the firmware API symbols at the addresses listed above.

---

## Summary of what remains NOT FOUND
- Envelope seconds and curves (beyond the ~3 s max attack and "rather linear", both SOS observations).
- Filter topology, low-cut frequency and drive curve.
- Prologue-specific per-wave SHAPE behaviour.
- VPM per-type ratio and depth tables, and any firmware reverse engineering.
- Noise filter orders and Decim bit depth.
- Duo, Sub and unison curves, and chord voicings.
- L.F. COMP time constants and frequency.
- Numeric velocity curves, the ARP RANGE byte mapping, the XFADE law, and portamento seconds.
- Factory effect algorithms.
- Whether the websim tables match the prologue's flash bit for bit, and how the unit signature works.
