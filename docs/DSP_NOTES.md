# DSP notes

Step 9 path. Superb still equals Good. Linphase stays off.

- Oversampling wraps global Drive through lenses, Cavity, and the ±1 rail clip. Mix and Listen are after downsample.
- Glow–SatHeavy (0–22) use RMS makeup vs a 0 dBFS 200 Hz sine. Facet/Wreck (23–33) peak-normalize.
- Routes: One, Stack, Split, Prism (complementary one-pole splits), Axis (M/S), Cavity (lens 0 forward, lens 1 in the delay).
- Ring is a damped delay at `1/cN_fb_hz`. Cavity delay is up to 2 s at OS rate.
- Heat adds ±12 dB to active lens drive. Spread widens One/Axis side; Prism tilts band levels.
- Listen: Wet, Dry (delayed), Delta (`wet-dry`), Caustic (Delta HP 1.2 kHz +6 dB).
- Motion: Orbit 0 in a ValueTree; Wiring slot 0 can add onto Mix. `glob_motion` scales depth.
- `--self-test` on the standalone exits 0/1.
