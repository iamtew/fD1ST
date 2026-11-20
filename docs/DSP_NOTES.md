# DSP notes

Step 2: One route, Glow, makeup, Drive, mix, OS Off / Fast (2×) / Good (8×). Superb still equals Good.

- Bypass copies input to output and still reports OS latency.
- Mix is `dry + (wet - dry) * mix`. Dry is delayed by OS latency.
- Oversampling wraps global Drive through Glow and the ±1 rail clip. Mix is after downsample.
- Glow is `tanh`. Makeup is a 0 dBFS 200 Hz sine RMS table vs drive, filled in `prepare`.
- Lens 0 off is dry through that stage; mute is silence.
- Linear phase stays off. Ring, Cavity, other styles, and other routes wait.
