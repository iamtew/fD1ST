# fD1ST

Product name: **fD1ST**  
Spoken / short: **DIST** (from **distortion**). The leading **f** is **fx**. Stylized `D1ST` is the same letters as DIST (`1` = I). Never “fist.”  
Tagline: **Watch the signal break.**  
Kind: stereo distortion / saturation effects plugin (product TBD)  
Framework: JUCE (Projucer, no CMake)  
Language: C++17  
Targets: VST3, Standalone  
I/O: stereo audio in, stereo audio out

The previous Path / Focus / Motion product was gutted. Forest Depth and brand stay. DSP and UI grow from the stub below.

---

## 1. Identity

Id: `forest-depth`  
Name: **Forest Depth**

Main **is** the plugin: burgundy chrome, not a black skin with a red logo.

Ship **only** these six hexes. Mix and alpha of them is allowed.

| Label | Hex | Role |
|-------|-----|------|
| Main | `#900020` | background, chrome, large surfaces |
| Deep Emerald | `#0F6B4E` | primary accent |
| Soft Sage | `#A8C5B0` | text, labels |
| Dark Forest | `#1A3C32` | recessed wells |
| Black | `#000000` | voids |
| White | `#FFFFFF` | peaks, readouts |

### Stack

- JUCE from `%USERPROFILE%\JUCE`. Projucer only.
- APVTS for host parameters.
- Host sample rates 44.1 / 48 / 88.2 / 96 / 176.4 / 192 kHz. Block sizes 32–2048.
- Denormal flush on the audio thread. No allocations on the audio thread after `prepareToPlay`.

### Non-goals

- AU, AAX, CLAP, VST2, iOS.
- Catalog tile on audio.stupid.systems until the Meat Bag asks.
- CMake.

---

## 2. Stub signal flow (step 0)

```
in stereo
  -> Out gain (out_db, default 0 dB)
  -> out stereo
```

Host bypass copies input to output.

---

## 3. Build order

- [x] **Step 0** — compiling VST3 + Standalone: burgundy editor, stereo thru, `out_db`, `--self-test` smoke, Forest Depth.
- Next steps: Meat Bag product SPEC. Do not invent Path / Lens / Motion again unless asked.
