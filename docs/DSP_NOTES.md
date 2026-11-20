# DSP notes

Step 1 is thru. Nothing here allocates, locks, or calls into the system on the audio thread.

- Bypass returns the input as the host sent it.
- Mix is `dry + (wet - dry) * mix`. Wet equals dry until Glow lands.
- Output is a smoothed dB gain after that.
- Sidechain bus is declared and ignored.
- Oversampling, linear phase, Ring, Cavity, and the 34 styles wait for later build-order steps in the spec.
