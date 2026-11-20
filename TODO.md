# fD1ST TODO

Spec: [docs/SPEC.md](docs/SPEC.md). Spoken name DIST. Never “fist.”

Each numbered step is a **compiling VST3**. Do not skip ahead to 32× FIR. v1 ship = through step 9 plus the open ceilings below. The build-order boxes are ticked; the plugin is **not** spec-complete until those ceilings are closed.

## Done

- [x] Step 1 — Just + `.jucer`, burgundy editor, stereo thru, full host APVTS, GitHub, Pages (`docs/`)
- [x] Step 2 — One route, Glow + makeup, Drive, mix, OS Off/Good
- [x] Step 3 — Transfer lens viz (CPU) + meters
- [x] Step 4 — Styles 1–33
- [x] Step 5 — Stack / Split / Prism / Axis / Cavity + Ring
- [x] Step 6 — Heat / Spread / Listen / Delta
- [x] Step 7 — Path graph + GPU
- [x] Step 8 — Motion ValueTree + one Orbit + Wiring
- [x] Step 9 — Remaining sources, factory JSON, `--self-test` (spec §11)
- [x] Catalog — stay off audio.stupid.systems until asked

## Open ceilings (do not forget)

Build-order steps 1–9 compiled, but these spec gaps remain. Close them before calling DIST done.

- [ ] Superb still equals Good — real 32× FIR OS
- [ ] Linphase stays off — enum present, UI disabled / treated as off
- [ ] Prism splits are complementary one-pole, not Linkwitz-Riley 24 dB
- [ ] Filter types 4–8 (Notch, Peak, Morph, Comb, Resample) are dry
- [ ] Bloom / FFT / Cavity phosphor are not drawn
- [ ] Motion extras are empty ValueTree nodes (Strike, Follow, MIDI, XY, Noise; Orbits 1–3; Wiring 1–49)
- [ ] Factory is host programs, not 48 `.fd1st` JSON files
- [ ] `--self-test` covers silence, bypass, and Glow makeup — not the full spec §11 list

## Meat Bag

- [ ] Replace `assets/logo.png` (placeholder) with the Affinity export, then `just icon` / `just docs-logo`
- [ ] `just docs-screenshots` once the editor is worth shooting
- [ ] First numeric tag (`0.0.1`, matches the jucer) when a build is worth shipping
- [ ] Catalog tile on Stupid Audio when ready to be public
