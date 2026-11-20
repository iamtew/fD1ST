# fD1ST

Product name: **fD1ST**  
Spoken / short: **DIST** (from **distortion**). The leading **f** is **fx** — this is an *effects* plugin, not an instrument. Stylized `D1ST` is the same letters as DIST (`1` = I). Never “fist.”  
Tagline: **Watch the signal break.**  
Kind: stereo distortion / saturation effects plugin  
Framework: JUCE  
Language: C++  
Targets: VST3, Standalone  
I/O: stereo audio in, stereo audio out, MIDI in, optional sidechain audio in (envelope / follower trigger)

This file is the implementation spec. Another Clanker should be able to build the plugin from this document without guessing.

fD1ST is **not** a collage of other products. Completeness of *controls* was checked against public saturator feature lists (appendix). Identity, workflow, language, graphics, visualization, and factory bank are original.

---

## 0. Manifesto

Distortion is light through a nonlinear **lens**. Drive is **pressure**. Frequency bands are a **prism**. Feedback is a **cavity**. New harmonics are **caustics** — the part of the sound that was not in the input.

The Meat Bag should always be able to **see** that: where the signal goes, how the waveform is bent, which overtones bloom, whether the cavity is breathing.

**Workflow (this is the product):**

1. **Path** — a living graph of the actual route. Click a node to work on it.
2. **Focus** — one **Lens** as a workbench: transfer curve, pressure, bias, filter, ring.
3. **Motion** — modulators attach *onto the path* (drop a source on a wire or a knob). A slot table exists for power users; it is not the front door.

**Three macros on the bar (not extra DSP engines):** **Heat**, **Spread**, **Motion**.  
**Signature listen:** **Wet / Dry / Delta / Caustic** — Delta is the grit you added; Caustic is that grit with the body stripped.

If the UI looks like a rack of stages plus a modulator strip at the bottom, it is wrong. If the Meat Bag can solo the *difference* and watch harmonics flare on the focused lens, it is right.

---

## 1. Identity, stack, non-goals, legal

### Who it is

A production-grade **distortion / saturation FX** that is also a signal observatory. Use it as quiet bus glue or as a wrecking cavity. The same Path/Focus/Motion chrome scales from one lens to six.

**Name:** `f` + `D1ST`. `D1ST` = **DIST** = **distortion**. Lowercase `f` = **fx** (effect, not instrument). Host category: effect / distortion. Do not brand it as “fist.”

### Color palette

Id: `forest-depth`  
Name: **Forest Depth**  
Tag: Natural · Balanced  
Description: Deep emerald and soft sage create a calm, grounded contrast against the burgundy. Feels organic and high-end.

**Main means the plugin is burgundy.** Editor background, window chrome, header, large fills = `#900020`. Not a black skin with a red logo. Emerald/sage sit *on* that field.

Ship **only** these six hexes. Mix and alpha of them is allowed (e.g. Main mixed with Black for inset plot wells; sage at 30% on Main for ghost FFT). Do not invent extra hues.

| Label | Hex | Role |
|-------|-----|------|
| Main | `#900020` | **the** plugin color: background, chrome, large surfaces |
| Deep Emerald | `#0F6B4E` | primary accent: wet / processed, selection, Motion, Heat readout |
| Soft Sage | `#A8C5B0` | secondary / text, dry / input ghost, labels on burgundy |
| Dark Forest | `#1A3C32` | recessed green wells, knob tracks, nested panels |
| Black | `#000000` | true voids, plot interiors, mix with Main for depth |
| White | `#FFFFFF` | peaks, value readouts, plot rails, max Heat |

```json
{
  "id": "forest-depth",
  "name": "Forest Depth",
  "tag": "Natural · Balanced",
  "description": "Deep emerald and soft sage create a calm, grounded contrast against the burgundy. Feels organic and high-end.",
  "colors": [
    { "label": "Main", "hex": "#900020", "role": "main / background / chrome" },
    { "label": "Deep Emerald", "hex": "#0F6B4E", "role": "primary accent" },
    { "label": "Soft Sage", "hex": "#A8C5B0", "role": "secondary / text" },
    { "label": "Dark Forest", "hex": "#1A3C32", "role": "recessed wells" },
    { "label": "Black", "hex": "#000000", "role": "voids / plot interiors" },
    { "label": "White", "hex": "#FFFFFF", "role": "peak / readouts" }
  ]
}
```

### Vocabulary (UI and docs)

DSP IDs may keep `cell` / `glob` / `xover` for stability. The editor **must** use these names:

| Internals | Meat Bag name |
|-----------|----------------|
| Routing 0–5 | **One / Stack / Split / Prism / Axis / Cavity** |
| Cell | **Lens** |
| Per-lens delay feedback | **Ring** |
| Global delay feedback | **Cavity** |
| Modulation | **Motion** |
| Matrix | **Wiring** (advanced) |
| Input / dry | **Soft Sage** `#A8C5B0` |
| Processed / wet | **Deep Emerald** `#0F6B4E` |
| Heat / pressure | **Deep Emerald** → **White** (must read on burgundy chrome) |
| Motion traces | **Deep Emerald** on sage/wet wires |

### Stack

- JUCE 8 (or current stable 7.x): `juce_audio_processors`, `juce_dsp`, `juce_gui_basics`, **`juce_opengl` required for the primary editor**. CPU fallback editor is reduced (see §8).
- Plugin processor + editor, `AudioProcessorValueTreeState` for automatable params.
- Host sample rates 44.1 / 48 / 88.2 / 96 / 176.4 / 192 kHz. Block sizes 32–2048.
- Denormal flush on audio thread. No allocations on the audio thread after `prepareToPlay`.

### Non-goals

- AU, AAX, CLAP, VST2, iOS.
- Cloning any vendor UI, logo, preset names, or trademarked amp names.
- Neural / IR amp captures.
- Surround, Ambisonics, >2 main channels (sidechain is extra stereo or mono).
- Decorative FFT with no transfer curve / path graph.

### Legal

Do not ship FabFilter, Ableton, or third-party trademarks. Do not reverse binaries. Styles below are original recipes (tanh, polynomials, fold, bit depth, delay smear, etc.), not claims of circuit identity.

---

## 2. Signal flow and routing

```
in stereo
  -> tap dry (latency-compensated)
  -> global Drive (linear gain)
  -> optional global Tone pre (color compensation, see §5)
  -> Path (1–6 lenses, topology glob_route)
  -> sum / blend of active lenses
  -> Cavity loop (delay + compressor in loop) mixed with sum
  -> global Compressor
  -> optional global Tone post
  -> Heat/Spread already applied as offsets into the above (not a separate stage)
  -> Output gain
  -> hard clip (±1.0 linear, 0.1 dB cubic softening at the rail)
  -> Listen mode (Wet / Dry / Delta / Caustic) then Dry/Wet mix
  -> out stereo
```

Plugin **Bypass** is host-style: copy input to output, keep reported latency.

Oversampling wraps Drive through hard clip. Listen + Dry/Wet are at host rate after downsample.

### Routing (`glob_route`)

| Value | UI name | Active lenses | How they combine |
|-------|---------|---------------|------------------|
| 0 | **One** | Lens 0 | Direct |
| 1 | **Stack** | 0 then 1 | `y = lerp(lens0(x), lens1(lens0(x)), glob_blend)` |
| 2 | **Split** | 0 and 1 | `y = lerp(lens0(x), lens1(x), glob_blend)` |
| 3 | **Prism** | 0 .. (`glob_bands`-1) | Complementary crossovers, sum bands |
| 4 | **Axis** | 0 = Mid, 1 = Side | Encode L/R → M/S, process, decode |
| 5 | **Cavity** | 0 = forward, 1 = feedback path | See §5 |

`glob_blend` (0–1, default 0.5) is ignored in One and Prism.

- **Stack:** `blend=0` → lens 0 only; `blend=1` → `lens1(lens0(x))` (full stack). Same lerp as the table.
- **Split:** `blend=0` → lens 0 only; `blend=1` → lens 1 only.

`glob_bands` is 1–6, default 3, only used in Prism. Adding a band splits the hovered/selected region; removing deletes selected. Unused lenses stay in state; skip `process` entirely when `cN_on=false`.

Channel mode (`glob_chmode`): 0 Stereo, 1 Axis **of the whole plugin** after routing (second M/S layer). Default Stereo. Drive pan and lens pan interpret as L/R or M/S depending on this flag.

**Double Axis:** if `glob_route` is Axis, **ignore** `glob_chmode` (already M/S). Do not encode twice.

Path UI always shows the *actual* graph for the current route (not six empty slots).

---

## 3. Lens DSP

A **lens** is one distortion stage / band. Six lenses, indices 0–5. Identical DSP. Code may still say `cell`.

Order inside a lens (per sample, oversampled domain):

1. If disabled or muted (and another lens is audible): pass dry band/stage input (Prism mute = silence that band).
2. Ring mix: `x = x + fb_delayed * cell_fb_amt`.
3. Dynamics (bipolar compress / expand) on `x`.
4. Filter if `cell_filt_pos == Pre`.
5. Drive gain + Heat offset + DC bias, waveshape (`cell_style`), auto makeup.
6. Filter if `cell_filt_pos == Post`.
7. Tone EQ (Bass / Mid / Treble / Presence).
8. Lens Mix vs dry tap (input to step 3).
9. Level + pan (+ Spread contribution, §5).
10. Write Ring delay.

**Lens solo:** if any lens is soloed, non-soloed lenses output silence (Prism) or are skipped (other routes). Multiple solos allowed. UI label: **Lens solo**.

**Enable vs mute vs mix vs solo**

| State | Audio |
|-------|--------|
| `cN_on=false` (disabled) | Dry through that stage/band. Prism: still in the complementary sum (dry band). Skip shaper CPU. |
| `cN_mute=true` | Silence that lens (zeros). Prism: hole in the sum. |
| `cN_mix=0` | Dry tap **after Ring input** (step 2 in), **no shaper/filter/tone**. Ring still sees input. |
| `cN_solo` | As above. Mute and disable are independent of solo. |

Init: `c0_on=true`, `c1_on`…`c5_on=false`. New Prism band: copy settings from the split lens, `on=true`, `cN_drive_db` layout default **12**. Init **program** sets `c0_drive_db=0` (layout default 12 is for newly added lenses, not Init).

### 3.1 Drive, bias, makeup

Let `g = db_to_lin(cell_drive_db + glob_heat * 12)` (Heat: −1..+1 → ±12 dB on every *active* lens). `b = cell_bias` in [-1, 1].

Drive pan (`cell_drive_pan`, -1..1): left/mid gets `g * (1 - p)`, right/side gets `g * (1 + p)` with `p = cell_drive_pan` scaled so 0 is equal.

Shaper input: `u = x * g + b * 0.25`.

**Makeup:** styles **0–22** auto-makeup so a 0 dBFS 200 Hz sine RMS stays roughly constant vs drive. `m = 1 / max(0.2, rms_estimate(style, g))`. Static table per style vs drive (fill in `prepare` from that sine), smoothed.

Styles **23–33** (Facet/Wreck): **peak-normalize only** (scale so a 0 dBFS sine peak after the shaper is ≤ 0 dBFS). Do not RMS-match Crush/Fold — that kills the character.

### 3.2 Dynamics (`cell_dyn`)

Range -1..+1, 0 = off.

- `d > 0`: downward compressor. Threshold -24 dBFS, ratio `1 + 7*|d|`, attack 8 ms, release 80 ms, makeup `1 + 0.4*|d|`. Soft knee 6 dB. RMS detector 5 ms.
- `d < 0`: expander/gate. Threshold -36 dBFS, ratio `1 + 5*|d|`, attack 2 ms, release 60 ms. Below threshold, gain tends toward `lin(-24 * |d|)` dB.

Stereo linked (max of L/R envelopes) unless Axis routing, then independent.

### 3.3 Filter (`cell_filt_type`)

| Id | Name | Recipe |
|----|------|--------|
| 0 | Off | Bypass |
| 1 | LP | SVF 12 dB/oct lowpass, Q = `cell_filt_q` |
| 2 | BP | SVF bandpass |
| 3 | HP | SVF 12 dB/oct highpass |
| 4 | Notch | SVF notch |
| 5 | Peak | SVF bell, ±12 dB from `cell_filt_q` mapped 0.3–8 → gain |
| 6 | Morph | Crossfade LP→BP→HP by `cell_filt_q` (0=LP, 0.5=BP, 1=HP) |
| 7 | Comb | Feedforward comb, delay = 1/`cell_filt_hz`, mix from Q |
| 8 | Resample | Sample-and-hold rate `cell_filt_hz` (100 Hz–Nyquist), no reconstruction filter |

`cell_filt_hz`: 20 Hz–20 kHz, default 1200.  
`cell_filt_q`: 0.3–8, default 0.707.  
`cell_filt_pos`: 0 Pre, 1 Post (default Post).

### 3.4 Tone EQ

Four peaking / shelf stages after the shaper (RBJ):

| Band | Type | Freq | Gain range | Default |
|------|------|------|------------|---------|
| Bass | low shelf | 120 Hz | ±12 dB | 0 |
| Mid | peak Q=0.8 | 800 Hz | ±12 dB | 0 |
| Treble | high shelf | 4 kHz | ±12 dB | 0 |
| Presence | peak Q=1.2 | 8 kHz | ±12 dB | 0 |

### 3.5 Ring (local feedback)

Delay line length `1 / cell_fb_hz` seconds (20 Hz–4 kHz, default 220 Hz). One-pole damping at 6 kHz in the loop. Amount 0–0.95 (UI 0–100%). Safety: if `glob_fb_safe` is on, clamp amount to 0.85 and peak-limit the delay write to ±0.98.

Pitch-like ringing, not tempo delay. Tempo delay is **Cavity**.

### 3.6 Mix, level, pan

- `cell_mix` 0–1, default 1 (wet).
- `cell_level_db` -inf..+36 dB, default 0. -inf via linear 0.
- `cell_pan` -1..1.

### 3.7 Style table (`cell_style`)

`x` is the driven sample `u`. Output is `y` then multiplied by makeup. Clip `y` to ±8 before later stages. All `tanh` is `std::tanh` or a Pade approx of equal quality.

**Tint** is for visualization only: how the transfer-lens stroke colors as drive rises (odd = Deep Emerald, even = White on emerald, digital = Soft Sage → White). Do not stroke curves in Main — they vanish on the burgundy chrome. Mix only palette hexes.

**Emulsion / glass (tube-like)**

| Id | Name | Tint | Recipe |
|----|------|------|--------|
| 0 | Glow | odd | `y = tanh(x)` |
| 1 | Clear | odd | `y = tanh(1.4*x) - 0.08*x*x*x` plus `|x|*x * 0.05` |
| 2 | Ember | even | `y = tanh(x + 0.25*x*x)` |
| 3 | Crack | mixed | `y = tanh(2.2*x) + 0.12*sin(3*x)` then 15 Hz HP |

**Print (tape-like)**

| Id | Name | Tint | Recipe |
|----|------|------|--------|
| 4 | Hissless | odd | `y = x / (1 + |x|)` |
| 5 | Print | even | `y = tanh(x) + 0.04*x*x`, LP 12 kHz |
| 6 | Pack | even | Print with `tanh(1.8*x)`, LP 8 kHz |
| 7 | Oxide | even | Pack + 0.3 ms wow (depth 0.15 ms, 0.4 Hz) + 2 kHz +2 dB |

**Coil (amp-like grit ladder, not brand names)**

| Id | Name | Tint | Recipe |
|----|------|------|--------|
| 8 | Tweedish | odd | Pre HP 80 Hz, `tanh(1.2*x)`, +3 dB @ 3.5 kHz |
| 9 | Stack | odd | Pre HP 100 Hz, two tanh (1.0 then 1.6), -2 dB @ 400 Hz |
| 10 | BritCrunch | odd | Stack, second `tanh(2.4*x)`, +4 dB @ 1.2 kHz |
| 11 | BritChime | odd | Tweedish + high shelf +3 dB @ 6 kHz |
| 12 | SmoothAmp | odd | `atan(x) * (2/π) * 1.2`, LP 7 kHz |
| 13 | CrunchAmp | mixed | `y = x / (1 + |x|^1.5)` |
| 14 | LeadAmp | odd | `tanh(2.8*x)` + 800 Hz +5 dB |
| 15 | ScreamAmp | digital | `y = tanh(sin(x*1.2)*2)` |
| 16 | PowerAmp | even | `tanh(x)` + 40 Hz HP |

**Iron (transformer-like)**

| Id | Name | Tint | Recipe |
|----|------|------|--------|
| 17 | TfLite | odd | DC block 20 Hz + `x / (1+0.3|x|)` |
| 18 | TfGentle | even | DC block 30 Hz + `tanh(0.9*x)` + 0.03 `x^2` |
| 19 | TfWarm | even | DC block 40 Hz + `tanh(1.3*x + 0.2*x*x)` |

**Pressure (generic sat)**

| Id | Name | Tint | Recipe |
|----|------|------|--------|
| 20 | SatLite | odd | `tanh(0.7*x)` |
| 21 | SatSoft | odd | `atan(x)*2/π` |
| 22 | SatHeavy | odd | `tanh(2.5*x)` |

**Facet (digital shapers)**

| Id | Name | Tint | Recipe |
|----|------|------|--------|
| 23 | SoftSine | odd | `y = sin(π/2 * clamp(x,-1,1))` |
| 24 | Diode | mixed | cubic then soft rail |
| 25 | Poly | odd | `y = x + 0.4*x^3 + 0.15*x^5` then tanh |
| 26 | TriFold | digital | triangle fold into [-1,1], ×0.9 |
| 27 | Shards | digital | quantize `3+12*(1-drive01)` levels, cubic tween |
| 28 | Inject | digital | `tanh(x)` + TPDF `* 0.08 * |tanh(x)|` |

**Wreck**

| Id | Name | Tint | Recipe |
|----|------|------|--------|
| 29 | Smear | mixed | 4 allpasses 1.1/1.7/2.3/2.9 ms, fb 0.4+0.4*drive01, tanh |
| 30 | Drop | mixed | 40 ms buffer, read rate `1-0.35*drive01`, 50% blend, tanh |
| 31 | Foldback | digital | classic fold into [-1,1] |
| 32 | Rectify | even | `abs` then 30 Hz HP then `tanh(1.5*y)` |
| 33 | Crush | digital | bits `4+12*(1-drive01)`, hold `fs/(1+drive01*32)` |

Default style: **Glow** (0).  
`drive01` = `cell_drive_db` mapped 0..36 dB → 0..1.

Diode full recipe: `y = x - 0.33*x^3` for `|x|<1` else `sign(x)*(0.67 + 0.1*tanh(3*(|x|-1)))`.

---

## 4. Crossovers, M/S, oversampling, linear phase

### Prism crossovers

`glob_bands` bands need `glob_bands - 1` splits.

- Split frequencies `xover_hz[i]`, 20 Hz–20 kHz, strictly increasing, ≥1/6 octave gap.
- Defaults for 3 bands: 200 Hz, 2 kHz. For 2 bands: 800 Hz. For 4+: geometrically space 80 Hz–8 kHz then user-edit.
- Slope `xover_slope[i]`: 6, 12, 24, 48 dB/oct (enum 0–3). Default 24.
- Complementary Linkwitz-Riley for 12/24/48. 6 dB/oct: complementary analog pair. **Sum of bands magnitude-flat ±0.2 dB from 20 Hz–Nyquist×0.45** (min-phase, sine sweep).
- Minimum-phase (default) or **linear-phase** (`glob_linphase`): FIR, latency reported. 2048-sample FIR at 48 kHz for 48 dB/oct; 512 taps at 6 dB/oct.

All `xover_hz` are Motion targets. Prism glass in the spectrum view is the same object as these splits.

### Axis encode/decode

```
M = 0.5*(L+R)
S = 0.5*(L-R)
; process
L = M+S
R = M-S
```

No extra gain. Axis routing uses this around lenses 0 and 1. `glob_chmode` Axis uses the same around the whole wet path.

### Oversampling (`glob_os`)

| Id | Name | Factor |
|----|------|--------|
| 0 | Off | 1 |
| 1 | Fast | 2 |
| 2 | Good | 8 |
| 3 | Superb | 32 |

Polyphase FIR. `glob_linphase` on → linear-phase OS always; off → min-phase OS allowed. Report `getLatencySamples()`. Default **Good**. Superb may be expensive.

**OS wrap:** oversampling covers **global Drive through hard clip**, inclusive. Ring and Cavity delay **times are in host seconds**, converted to OS samples (`delaySamples = time * sampleRate * osFactor`). Listen, Dry/Wet, bypass: **after** downsample.

Until build-order step 5 is green: **Superb may equal Good**, **linphase UI disabled / treated as off**. Enums still exist.

---

## 5. Global pressure, cavity, listen, macros

### Drive

`glob_drive_db` -12..+36 dB, default 0. Before the Path.

### Tone + color compensation

`glob_tone` -1..+1, default 0. Tilt, corner 800 Hz, ±8 dB at extremes.

`glob_colorcomp` bool, default on: **-tone** before lenses, **+tone** after. Off = post-only.

### Heat (`glob_heat`)

-1..+1, default 0. Adds `heat * 12` dB to **every active lens drive** (not global input Drive). Makeup tables follow the effective drive. Viz: Path nodes get brighter. Does not change inactive lenses.

### Spread (`glob_spread`)

-1..+1, default 0. No extra engine.

- **One / Stack / Split / Cavity:** extra `cell_pan` / `drive_pan` push: lens 0 toward L/Mid, lens 1 toward R/Side by `spread * 0.5`. One-route: width via M/S gain `S *= 1+spread` (decode after).
- **Prism:** extra level: low bands `+spread*3 dB`, high bands `-spread*3 dB` (or reverse if spread < 0).
- **Axis:** `S *= 1 + spread` after lens 1 (spread > 0 = wider grit).

### Motion macro (`glob_motion`)

0..2, default 1. Multiplies **all Wiring slot depths**. 0 = freeze modulators visually still running at depth 0. 2 = double.

### Listen (`glob_listen`)

| Id | Name | Output |
|----|------|--------|
| 0 | Wet | Normal wet, then `glob_mix` with dry |
| 1 | Dry | Latency-compensated input (mix ignored) |
| 2 | Delta | `wet - dry` (the added products). If mix is 0, Delta is silence |
| 3 | Caustic | Delta through 12 dB/oct HP @ 1.2 kHz, makeup +6 dB |

Listen is automatable. UI: four exclusive buttons. Editor keys: `W` Wet, `Shift+D` Dry, `E` Delta, `C` Caustic. **Δ is not a key.**

### Cavity (global feedback)

Always after lens sum, except route **Cavity** which also feeds lens 1:

**Route ≠ Cavity**

```
sum = lenses(x)
fb  = delay(compressor(sum + glob_fb_amt * fb_out))
y   = sum + glob_fb_amt * fb
```

**Route = Cavity**

```
fwd = lens0(x)
fb_in = x + glob_fb_amt * fb_out
fb_shaped = lens1(fb_in)
fb_out = delay(compressor(fb_shaped))
y = lerp(fwd, fb_out, glob_blend)
```

| Id | Mode | Time |
|----|------|------|
| 0 | Time | `glob_fb_time` 0.5 ms–2 s |
| 1 | Sync | note × `glob_fb_div` (1/32..1/1) |
| 2 | Triplet | Sync × 2/3 |
| 3 | Dotted | Sync × 1.5 |
| 4 | Note | `1 / glob_fb_note_hz` (27.5–4186 Hz) |

`glob_fb_amt` 0–1, default 0.

**Delay:** max **2 s at host sample rate** (not OS SR). Hermite interpolate reads. Tempo from host playhead; **if not playing, 120 BPM**.

**Sync divisions** (`glob_fb_div`): 1/32, 1/16, 1/8, 1/4, 1/2, 1/1 (straight). Triplet = ×2/3 of that note; Dotted = ×1.5.

`glob_fb_safe` default on: clamp amt 0.85, loop peak -0.3 dBFS, HP 15 Hz. Off: still HP 15 Hz, no amt clamp.

### Global compressor, out, mix

- `glob_comp` 0–1, default 0. Threshold -18 dBFS, ratio 1+7*amount, attack 5 ms, release 80 ms, SC HPF `glob_comp_hpf` default on @ 120 Hz on detector only. **Same knob** feeds the **cavity loop compressor and the output compressor**. One amount on purpose (cavity “breathes” with the output).
- `glob_out_db` -24..+24, default 0
- Hard clip after out
- `glob_mix` 0–1, default 1 (after Listen, except Dry)
- `glob_bypass` bool

A/B morph: `glob_ab_morph` 0–1 (UI, not necessarily a host param) interpolates A→B continuous params while held; enums stay on the nearer snapshot. Copy A→B.

---

## 6. Parameter list

All continuous params automatable, smoothed 8–20 ms except bypass, enums, bools. IDs are APVTS strings.

**Global**

| ID | Type | Range | Default | Notes |
|----|------|-------|---------|-------|
| glob_route | choice | 0–5 One…Cavity | 0 | |
| glob_blend | float | 0–1 | 0.5 | |
| glob_bands | int | 1–6 | 3 | Prism |
| glob_chmode | choice | Stereo, Axis | Stereo | |
| glob_drive_db | float | -12–36 | 0 | dB |
| glob_tone | float | -1–1 | 0 | |
| glob_colorcomp | bool | | true | |
| glob_heat | float | -1–1 | 0 | lens drive offset |
| glob_spread | float | -1–1 | 0 | |
| glob_motion | float | 0–2 | 1 | slot depth scale |
| glob_listen | choice | Wet Dry Delta Caustic | Wet | |
| glob_fb_mode | choice | 0–4 | 0 | |
| glob_fb_time | float | 0.0005–2 | 0.25 | s, log |
| glob_fb_div | choice | 1/32..1/1 | 1/8 | |
| glob_fb_note_hz | float | 27.5–4186 | 110 | log |
| glob_fb_amt | float | 0–1 | 0 | |
| glob_fb_safe | bool | | true | |
| glob_comp | float | 0–1 | 0 | |
| glob_comp_hpf | bool | | true | |
| glob_out_db | float | -24–24 | 0 | |
| glob_mix | float | 0–1 | 1 | |
| glob_os | choice | 0–3 | 2 Good | |
| glob_linphase | bool | | false | |
| glob_bypass | bool | | false | |

**Crossovers** (5 slots; unused ignored)

| ID | Type | Range | Default |
|----|------|-------|---------|
| xover_hz_1 .. xover_hz_5 | float Hz log | 20–20000 | 200, 2000, 5000, 8000, 12000 |
| xover_slope_1 .. xover_slope_5 | choice | 6/12/24/48 | 24 |

**Per lens n = 0..5** (prefix `cN_`)

| ID | Type | Range | Default |
|----|------|-------|---------|
| cN_on | bool | | true for n=0 only at Init |
| cN_solo | bool | | false |
| cN_mute | bool | | false |
| cN_style | choice | 0–33 | 0 Glow |
| cN_drive_db | float | 0–36 | 12 |
| cN_drive_pan | float | -1–1 | 0 |
| cN_bias | float | -1–1 | 0 |
| cN_mix | float | 0–1 | 1 |
| cN_dyn | float | -1–1 | 0 |
| cN_bass | float | -12–12 | 0 |
| cN_mid | float | -12–12 | 0 |
| cN_treble | float | -12–12 | 0 |
| cN_presence | float | -12–12 | 0 |
| cN_filt_type | choice | 0–8 | 0 Off |
| cN_filt_hz | float log | 20–20000 | 1200 |
| cN_filt_q | float | 0.3–8 | 0.707 |
| cN_filt_pos | choice | Pre, Post | Post |
| cN_fb_amt | float | 0–1 | 0 |
| cN_fb_hz | float log | 20–4000 | 220 |
| cN_level_db | float | -60–36 | 0 |
| cN_pan | float | -1–1 | 0 |

**Host APVTS (VST3 params):** globals in the table above + all `cN_*` + xovers. That is the automatable surface.

**ValueTree only (not host params — hosts choke at 250+):** Motion sources and 50 Wiring slots. Meat Bag automates them by wiring onto existing APVTS IDs.

Sources are **always allocated**. No add/remove. Unused = not wired (depths 0).

- 4 Orbit (`orb0`…`orb3`), 4 Strike (`stk0`…`stk3`), 4 Follow (`fol0`…`fol3`), 2 MIDI (`mid0` `mid1`), 6 XY (`xy0`…`xy5`), 2 Noise (`nz0` `nz1`).
- 50 slots: `src`, `dst` (APVTS string ID), `depth` -1..+1, `on`, `lag` 0–200 ms.
- Slot `src` = `{type, index}` e.g. `orb:0`.

**One Orbit** (×4, prefix `orbN_`):

| Key | Type | Range | Default |
|-----|------|-------|---------|
| wave | choice | sine tri sawUp sawDown square snh step16 | sine |
| morph | float | 0–1 | 0 |
| rate_hz | float log | 0.01–40 | 1 |
| sync | bool | | false |
| div | choice | same as glob_fb_div | 1/8 |
| phase | float | 0–1 | 0 |
| freq_off | float | -0.5–0.5 | 0 |
| balance | float | -1–1 | 0 |
| trig | choice | off retrig legato | off |
| step[16] | float | -1–1 | 0 |
| step_glide | float | 0–1 | 0 |

Strike (×4 `stkN_`): A, D, S, R, curveA/D/R, trig (midi/main/sc/lens0–5), thresh, range (normal/neutral), retrig.

Follow (×4 `folN_`): mode loudness/transient, atk, rel, thresh, gain, det_hz, det_width, src (main/sc/lensN).

MIDI (×2): kind CC/vel/note/bend/pressure, cc 0–127.

XY (×6): x, y, unipolar, slider.

Noise (×2): kind simplex/wander/snh/brown, rate_hz, sync, div, smooth.

Non-automatable UI state: editor size, fullscreen, A/B snapshots, MIDI-learn map, undo stack, viz overlay toggles (bloom/path/vectors) persisted per user not per preset.

Do not modulate: `glob_os`, `glob_linphase`, `glob_route`, `glob_bands`, `glob_listen`, style enums, filter type, fb mode.

---

## 7. Motion, MIDI, sidechain

Front door: **drag a source from the Motion well onto a Path wire or a Focus knob.** That creates a Wiring slot. Depth is a Deep Emerald ring; drag the ring.

**Wiring** (advanced table, 50 slots): source, dest ID, depth -1..+1, enabled, lag 0–200 ms. Additive in normalized space, then clamp. `glob_motion` scales depths after that.

Sources may modulate other source params (rate, attack). Not discrete params (see §6).

### XLFO (×4) — UI: **Orbit**

- Wave: sine, tri, saw up, saw down, square, s&h, **16-step** (levels -1..1, per-step glide).
- Morph 0–1. Rate free 0.01–40 Hz or sync. Phase 0–1. Stereo `freq_off` -0.5..0.5.
- Balance -1..1. MIDI trigger: off / retrigger / legato.

### EG (×4) — UI: **Strike**

- ADSR: A 0.1 ms–5 s, D 1 ms–5 s, S 0–1, R 1 ms–10 s. Curve per A/D/R -1..+1.
- Trigger: MIDI, main, sidechain, lens `n` input.
- Threshold -60..0 dB. Range: Normal (0..1) or Neutral sustain.

### Follower (×4) — UI: **Follow**

- Loudness (RMS 10 ms) or transient. Attack 0.5–200 ms, release 5–2000 ms.
- Detection BP does not affect audio. Listen-detect: editor hold only, not saved on.
- Source: main / sidechain / lens n.

### MIDI (×2), XY (×6), Noise (×2)

Same capabilities as before: CC/vel/note/bend/pressure; XY uni/bipolar or slider; Noise simplex / wander / S&H / brown, rate + smooth.

### MIDI learn

Learn, wiggle CC, bind last focused param. Persist. Omni default.

### Sidechain

VST3 extra stereo (or mono duplicated). EG/Follow trigger. Not mixed into audio unless detect-listen.

---

## 8. Path, Focus, Motion — UI and visualization

This section is load-bearing. A plugin with all DSP but a rack-of-knobs editor **fails the spec**.

### Chrome

```
[ preset name | undo redo | A/B morph | fD1ST | Heat Spread Motion | W / Shift+D / E / C | MIDI | OS | fullscreen ]
[ Living Path  — graph for current route, flow, click to Focus ]
[ Focus workbench — Transfer lens + Harmonic bloom + lens knobs ]
[ Prism spectrum / Cavity ring / Vectors  — context strip ]
[ Motion well — Orbits Strikes Follows Noise XY  |  Wiring table (collapsed) ]
```

Default size **1100×720**, min **900×560**, scale 0.8 / 1.0 / 1.25 / 1.5 / 2.0 + user drag. Fullscreen.

Launch Focus: **Drive** (global) until a lens node is clicked. Transfer curve still draws **lens 0** in One (that is the shaper).

Viz numbers: transfer LUT **512**, rider **N=64**, RMS probe **20 ms**, FFT **4096**, hop **~10 ms**.

**Color:** Forest Depth only (§1). The editor **is** **Main** `#900020` — full background and chrome. Recessed wells **Dark Forest** `#1A3C32`. Plot interiors mix Main with **Black**. Dry/input **Soft Sage**. Wet/selection/Motion/Heat **Deep Emerald**, Heat to **White** at extremes. Prism glasses low→high: Dark Forest / Deep Emerald / Soft Sage on the burgundy field. No extra hexes, no black-skin-with-red-accent, no skeuomorphic amps, no cool-grey scientific clone.

Knobs: vertical drag, wheel, double-click value, Shift fine, Ctrl/Cmd-click default. Multi-lens select: relative continuous, absolute discrete.

Undo/redo 50 steps in-plugin. A/B snapshots + morph slider.

### 8.1 Living Path (always on)

Nodes, left→right (or top→bottom on narrow scale): **In → Drive → [lenses as topology] → Cavity → Comp → Clip → Out**.

- Wire **thickness and brightness** = short RMS at that node (probe, 20 ms).
- Sage ghost wire = dry.
- Click node → Focus that lens (or global section).
- Prism: lenses sit as stacked glass; drag splits on the spectrum *or* on the stack.
- Axis: two rails labeled Mid / Side.
- Stack: two nodes in series; Split: two in parallel with a blend diamond.
- Cavity node **pulses** at delay period; radius = `glob_fb_amt`.
- Comp node **squeezes** height = gain reduction.
- Emerald ticks on wires = active Motion routes.

If RMS is silent, Path still draws (dim), never an empty box.

### 8.2 Transfer lens (Focus)

Large I/O plot, ±1.5 on both axes, 45° dry line in Soft Sage. Rails White at low alpha on Black.

- Stroke = current style LUT including drive, Heat, bias (same function as audio, 256–512 points, rebuilt when those params change).
- **Input density:** histogram of recent `u` samples under the X axis (glow).
- **Output density:** histogram of `y` on the Y axis.
- **Rider:** last N instantaneous `(u,y)` dots decaying (phosphor).
- Tint from style table: odd/even/digital mix on the stroke.
- Bias shifts the curve off-origin visibly.

This is how the Meat Bag understands *what the shaper is doing*. Not optional.

### 8.3 Harmonic bloom

Beside the transfer lens. H1–H16 of the **focused lens output** (or plugin output if Focus is global).

- Tracked pitch: YIN or autocorrelation on the probe buffer; if unvoiced, show noise-floor bars (Soft Sage at low alpha).
- Odd harmonics one hue, even the other (matches tint).
- Radial **caustic** mode (default) or bar mode toggle.
- Delta listen: bloom of residual only (the point of Caustic).

Probe: 2048 samples, hop ~10 ms, UI thread. No extra oscillator injected into the audible path.

### 8.4 Prism spectrum

Log FFT 20 Hz–20 kHz, 4096, Hann. **Input sage fill (ghost, ~30% on plot void), output Deep Emerald line; peaks White.** Toggle **Δ spectrum** (output minus input, clipped at 0).

In Prism route, band regions are draggable glass matching `xover_hz` / slope. Click glass = Focus that lens. `+` splits at mouse frequency. Alt-drag level handle = drive. Wheel = level.

### 8.5 Cavity ring

Visible when `glob_fb_amt > 0.01` or route is Cavity: circular delay buffer, write head, loop level as ring thickness, compressor GR as a dark sector. Time/Sync/Note mode labeled in the hole.

### 8.6 Vectors

Small Lissajous + correlation meter. Required in Axis and when `|Spread| > 0.05`; otherwise collapsible.

### Quality bar

- **60 fps** while editor open and host is playing or UI is dragging; cap 30 fps if CPU viz fallback; **sleep** when editor hidden (no GPU, no probes beyond a tiny RMS).
- **GPU primary** (JUCE OpenGL / OpenGL 3.3). MSAA 4x on the transfer lens. Additive bloom on rider dots, not a full-screen fake HDR stack.
- **CPU fallback:** transfer lens (software path), Path as static graph + numeric RMS, spectrum 15 fps, no bloom, no cavity phosphor. Banner: `GPU off — reduced sight`.
- Audio thread: write probes into lock-free SPSC (curve dirty flag, FFT time-domain window copy already downsampled, per-node RMS, GR, delay write index). **Never draw. Never alloc.**

---

## 9. Factory bank

Loader: `.fd1st` JSON in resources. User dir: `File::getSpecialLocation(userApplicationDataDirectory)/fD1ST/Presets`.

Preset change: continuous params interpolate 20 ms; enums snap.

Do not use other vendors’ preset names. Section presets = **lens stamps** (`cN_*` except solo/mute).

### Lens stamps (ship 12)

Glow, Ember, Print, Pack, SatLite, TfWarm, CrunchAmp, SoftSine, Foldback, Crush, Smear, Rectify — each is Init-like One-route with that style, drive 12 dB, mix 100%.

### Factory presets (ship all 48)

Columns: name, route, gist (enough to build; implementer fills exact JSON from this).

**Init / sight**

| # | Name | Route | Build |
|---|------|-------|--------|
| 1 | Init | One | Glow, drive 0, mix 100, OS Good, Heat/Spread/Motion 0/0/1, listen Wet |
| 2 | SeeDelta | One | SatHeavy drive 18, mix 100. Saved listen Wet; hit `E` for Delta |
| 3 | QuietMaster | One | SatLite drive 4, mix 35, linphase on, OS Superb, glob_comp 0.15 |
| 4 | MakeupMatch | One | Glow drive 24 vs Init — for checking auto makeup; mix 100 |

**Bus / print**

| # | Name | Route | Build |
|---|------|-------|--------|
| 5 | SoftPress | One | SatLite 6 dB, mix 40, glob_comp 0.22 |
| 6 | NightPrint | One | Print 8 dB, tone -0.2, colorcomp on |
| 7 | PackThe2b | One | Pack 10 dB, glob_comp 0.3, mix 70 |
| 8 | IronBus | One | TfWarm 7 dB, presence +2, mix 50 |
| 9 | GluePress | Split | L0 Glow 5 dB, L1 SatHeavy 12 mix 25, blend 0.3, Heat +0.1 |

**Drums**

| # | Name | Route | Build |
|---|------|-------|--------|
| 10 | KickWeight | Prism 2 | split 180 Hz; low Ember 14 + dyn +0.35; high Clear 4 mix 40 |
| 11 | SnareCrack | One | CrunchAmp 16, HP 180 pre, presence +4 |
| 12 | RoomSmash | Split | L0 Pack 8, L1 Crush 10 mix 18, blend 0.25 |
| 13 | HatGlass | One | Clear 10, HP 6 kHz pre, mix 60 |
| 14 | KitPrism | Prism 3 | 180 / 2500; Glow / Ember / Clear; drives 8/12/6; dyn mid +0.2 |

**Bass**

| # | Name | Route | Build |
|---|------|-------|--------|
| 15 | SubKeep | Prism 2 | split 90 Hz; low SatLite 3 mix 30; high Ember 14 |
| 16 | EightOhGrit | One | TfWarm 18, HP 40, glob_comp 0.25 |
| 17 | MonoCore | Axis | Mid Glow 8; Side mute; Spread -0.4 |
| 18 | GrowlCoil | One | LeadAmp 20, comb filt 110 Hz post Q 3, Ring 0.2 @ 110 |

**Voice / strings / guitar**

| # | Name | Route | Build |
|---|------|-------|--------|
| 19 | VocalSilk | One | Glow 5, mix 45, tone +0.1 |
| 20 | VocalBite | Split | L0 Clear 4; L1 Diode 14 HP 1.2k pre mix 20; blend 0.35 |
| 21 | AirWire | One | BritChime 12, presence +5, mix 50 |
| 22 | AmpInTheRoom | Stack | L0 Tweedish 10; L1 PowerAmp 8; blend 0.55 |
| 23 | AcousticEmber | One | Ember 6, mix 30, glob_comp 0.1 |

**Prism color**

| # | Name | Route | Build |
|---|------|-------|--------|
| 24 | ThreeGlass | Prism 3 | 200 / 2k; Glow / Ember / Clear; 8/10/6 |
| 25 | SixShard | Prism 6 | geometric xovers; alternate Glow and Shards on highs; Heat +0.2 |
| 26 | LowLaw | Prism 3 | steep 48 dB; low Crush 6 mix 20; mid Print 8; high off-ish mix 15 |
| 27 | HighLace | Prism 3 | high Foldback 8 mix 35; low/mid SatLite 4 |
| 28 | SlopeWall | Prism 4 | 80/400/4k; 48 dB; dyn +0.15 all |

**Axis**

| # | Name | Route | Build |
|---|------|-------|--------|
| 29 | MidKind | Axis | Mid Glow 4; Side CrunchAmp 12 mix 70; Spread +0.3 |
| 30 | SideKnives | Axis | Mid SatLite 2; Side Shards 16 HP 2k |
| 31 | CenterPunch | Axis | Mid SatHeavy 14 dyn +0.3; Side Glow 2 |

**Cavity**

| # | Name | Route | Build |
|---|------|-------|--------|
| 32 | EighthGhost | Cavity | Sync 1/8, amt 0.32, glob_comp 0.4, L1 Ember |
| 33 | DottedTrail | Cavity | Dotted 1/8, amt 0.28, L1 Oxide |
| 34 | NoteDrone | Cavity | Note 110 Hz, amt 0.45, safe on, L1 Glow Ring 0.3 @ 110 |
| 35 | BreathBack | Cavity | Time 180 ms, amt 0.55, glob_comp 0.65 (breathing GR) |
| 36 | TripletSmear | Cavity | Triplet 1/8, L1 Smear, amt 0.3 |

**Motion**

| # | Name | Route | Build |
|---|------|-------|--------|
| 37 | FollowHeat | One | Glow 10; Follow → glob_heat depth 0.6, detect 80–200 Hz |
| 38 | OrbitPrism | Prism 3 | Orbit 1/4 sine → xover_hz_1 depth small; Orbit2 → mid drive |
| 39 | NoiseBias | One | Ember; Noise wander → c0_bias depth 0.4, Motion 1.2 |
| 40 | StrikeOpen | One | Pack 8; Strike MIDI → glob_mix 0→1, 80 ms attack |
| 41 | TransientOpen | One | Follow transient → glob_mix inverted so hits go cleaner |
| 42 | XYPerform | One | XY0 → Heat / Spread; ready for mouse |

**Wreck**

| # | Name | Route | Build |
|---|------|-------|--------|
| 43 | FoldTheSun | One | Foldback 22, OS Good |
| 44 | BitRain | One | Crush 24, Resample filt 9 kHz post |
| 45 | DropOut | One | Drop 18, mix 80 |
| 46 | ShardChoir | Split | L0 SoftSine 8; L1 Shards 20; blend 0.4 |
| 47 | RectNight | One | Rectify 16, Ring 0.35 @ 55 Hz |
| 48 | FullWreck | Stack | L0 Crush 20; L1 ScreamAmp 14; Cavity amt 0.2 Time 90 ms; Heat +0.35 |

---

## 10. CPU, threads, safety

- Audio thread: process, smoothing, Motion at block rate (vector OK if LFO < 40 Hz; 16-step interpolate inside block). Probe writes only.
- UI / GL thread: consume probes, draw Path/lens/bloom/FFT.
- `prepareToPlay`: delay lines (2 s × os × 2 ch × 6 Ring + 1 Cavity), OS filters, FFT windows, LUT tables.
- DC: every offset-capable shaper HP ≥ 15 Hz.
- NaN: `!isfinite` → zero sample, skip lens rest of block.
- Denormals: `ScopedNoDenormals`.
- Latency: OS + linphase FIR. Dry path delayed to match so Delta is valid.

---

## 11. Acceptance tests

Standalone `--self-test` exit 0/1.

1. **Silence in, silence out** (Init, no cavity): RMS < -90 dBFS.
2. **Bypass**: max error 1e-5 after latency.
3. **Drive makeup**: 200 Hz -12 dBFS sine, Glow, drive 0 vs 24 dB, output RMS within 3 dB.
4. **Prism sum**: 3 bands, mix 0 all lenses, white noise, 20 Hz–16 kHz ±0.5 dB vs input (min-phase, OS off).
5. **No DC**: Rectify, 40 Hz 0 dBFS, |mean| < -60 dBFS after 0.5 s.
6. **Axis roundtrip**: both lenses mix 0, L≠R, out vs in ±0.1 dB.
7. **Cavity safety**: route Cavity, amt 1, safe on, 1 s noise; peak ≤ 0 dBFS + 0.5 dB; no NaN.
8. **Oversample alias**: 18 kHz sine @ 48 kHz, Foldback, Off vs Good; image ≥ 20 dB down with Good.
9. **Wiring**: Orbit depth 1 onto glob_mix, 1 Hz; effective mix oscillates.
10. **State**: save/load restores params including 50 slots.
11. **Delta silent at mix 0**: wet==dry, listen Delta, RMS < -80 dBFS.
12. **Heat**: Init, set all active lens drives 12 dB, Heat +1; effective drive 24 dB ±0.1 (solo one lens sine RMS matches drive 24 reference).
13. **Probes**: `processBlock` heap alloc count 0 after prepare (debug allocator or hook).
14. **Listen Dry**: equals delayed input, mix ignored.
15. **Stack blend 0**: equals lens 0 only (±1e-5).
16. **Axis route**: `glob_chmode` Mid-Side vs Stereo, bit-similar output (chmode ignored).
17. **Tempo fallback**: no playhead / not playing, Sync 1/4 delay = 0.5 s at 120 BPM (±1 sample at host SR).
18. **Inactive lens**: `c1_on=false` on One route — `LensProcessor::process` for lens 1 is not called (or equivalent skip).

---

## 12. Implementation notes (JUCE)

- Buses: stereo in, stereo out, optional `"Sidechain"`.
- Accept MIDI.
- `createParameterLayout()` in one place. `LensProcessor` with `prepare` / `reset` / `process`.
- Std JUCE DSP only: `Oversampling`, `StateVariableTPTFilter`, `IIR`, `DelayLine`; write the loop compressor if `juce::dsp::Compressor` cannot sit in the cavity cleanly.
- Editor: `OpenGLContext` attached to the main viz component. CPU fallback if context fails.
- IDs: §14 (`fD1ST` / `Fdst` / `Fd1s`). Spoken name **DIST**.

---

## 13. Completeness checklist (not the design brief)

Public pages used only to ensure we did not *drop* a useful control class. They are not a skin, a layout, or a preset list to imitate.

- https://www.fabfilter.com/products/saturn-2-multiband-distortion-saturation-plug-in
- https://www.fabfilter.com/help/saturn/using/overview
- https://www.fabfilter.com/help/saturn/using/bandcontrols
- https://www.fabfilter.com/help/saturn/using/modulation
- https://www.fabfilter.com/downloads/pdf/help/ffsaturn2-manual.pdf
- https://www.ableton.com/en/packs/roar/
- https://www.ableton.com/en/blog/roar-meet-live-12s-new-processing-powerhouse/

| Control class from public saturators | Where it lives in fD1ST |
|--------------------------------------|-------------------------|
| Many bands, slopes, lin-phase, 8×/32× OS | Prism, glob_os, glob_linphase |
| Per-band drive, mix, ring, dyn, tone, level, solo | Lens |
| Style families | Original IDs 0–33 + tints |
| Deep modulation | Motion / Wiring |
| Six topologies | One Stack Split Prism Axis Cavity |
| Per-stage bias + filter pre/post | Lens |
| Timed feedback + compressor in loop | Cavity |
| Global drive, tone, clip, dry/wet | §5 + Listen Delta/Caustic |

Identity that those products do **not** define and fD1ST **must** ship: Path→Focus→Motion, Heat/Spread/Motion macros, Delta/Caustic listen, transfer-lens + harmonic bloom + living path, factory names in §9.

---

## 14. Project contract

Spec lives in `docs/SPEC.md`. Code lives in this repo (`~/src/fD1ST`).

| Item | Value |
|------|--------|
| JUCE | 8 |
| Language | C++17 |
| Build | **Just** (`justfile`). No CMake. JUCE Projucer `.jucer` → Visual Studio 2022 (Win64). Recipes: `just build`, `just self-test`, `just clean`. |
| Formats | VST3 + Standalone |
| v1 OS | **Windows x64** (Meat Bag’s box). macOS later, not v1. |
| Plugin name | `fD1ST` |
| Description | DIST distortion FX |
| Manufacturer | `Stupid Audio` |
| Manufacturer code | `Ssys` |
| Plugin code | `Fd1s` |
| VST3 category | `Fx\|Distortion` |
| Font | JUCE default. No bundled font. Sage labels on Main; White values on Main. |
| Help | One overlay: manifesto + listen keys. |
| Undo | **Editor-local** (50 steps). Do not fight host undo. |
| Preset file | JSON `{ "version": 1, "params": {}, "motion": {} }` |

Change IDs only if a clash appears at register time.

### Build order

Each step is a **compiling VST3**. Do not skip ahead to 32× FIR.

1. Just + `.jucer` skeleton, burgundy editor, stereo thru, APVTS globals + lens 0.
2. One route, Glow + makeup, Drive, mix, OS Off/Good.
3. Transfer lens viz (CPU) + meters.
4. Styles 1–33.
5. Stack / Split / Prism / Axis / Cavity + Ring.
6. Heat / Spread / Listen / Delta.
7. Path graph + GPU.
8. Motion ValueTree + one Orbit + Wiring.
9. Remaining sources, factory JSON, `--self-test`.

**v1 ship = through step 9.** Until step 5 is green: Superb may alias Good; linphase stays off (enum present, UI disabled).
