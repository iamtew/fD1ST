# Architecture

PluginProcessor owns the APVTS (globals + six lenses + five crossovers). Motion and Wiring live on a `MOTION` ValueTree, not host params.

Audio thread: denormal flush, optional OS (Off/Fast/Good/Superb, IIR or FIR), Drive, Tone, Path, Cavity, Comp, clip, then host-rate Listen and Mix against a latency-matched dry tap.

The editor is Forest Depth burgundy: Path, Focus (transfer, bloom, spectrum, cavity), Heat/Spread/Motion macros, per-lens workbench, Wiring well. OpenGL is attached for the 2D viz; the CPU path still paints if the context is gone.

## Files

- `src/dsp/Names.h` — choice labels and counts. JUCE-free for `just check`.
- `src/dsp/LensProcessor.h` — one lens: style, filter, ring, dyn, EQ.
- `src/dsp/Motion.h` — sources and 50 wires.
- `src/parameters/ParameterIDs.h` — APVTS string IDs. Do not rename.
- `src/parameters/ParameterLayout.h` — the host surface.
- `src/presets/Factory.h` — 48 spec programs, 12 stamps, `.fd1st` JSON.
- `src/ui/Theme.h` — Forest Depth, six hexes.
