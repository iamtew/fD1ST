# DSP notes

Spec path. Superb is 32×. Linear phase selects FIR oversampling and FIR Prism splits.

- Oversampling wraps global Drive through lenses, Cavity, and the ±1 rail clip. Mix and Listen are after downsample.
- Glow–SatHeavy (0–22) use RMS makeup vs a 0 dBFS 200 Hz sine. Facet/Wreck (23–33) peak-normalize.
- Routes: One, Stack, Split, Prism (6 dB complementary one-pole, 12 dB two-pole, 24/48 Linkwitz-Riley), Axis (M/S), Cavity (lens 0 forward, lens 1 in the delay).
- Ring is a damped delay at `1/cN_fb_hz`. Cavity delay is up to 2 s at OS rate, Time/Sync/Triplet/Dotted/Note, Hermite reads.
- Heat adds ±12 dB to active lens drive. Spread widens One/Axis side; Prism tilts band levels.
- Listen: Wet, Dry (delayed), Delta (`wet-dry`), Caustic (Delta HP 1.2 kHz +6 dB).
- Motion: 4 Orbit, 4 Strike, 4 Follow, 2 MIDI, 6 XY, 2 Noise, 50 Wiring slots. `glob_motion` scales depth.
- `--self-test` on the standalone exits 0/1 (spec §11).
