# Architecture

PluginProcessor owns the APVTS. Step 1 has no Path DSP: `processBlock` is stereo thru with denormal flush, bypass, mix (noop while wet equals dry), and output gain.

The editor is a burgundy chassis with Mix, Output, Lens 0 Drive, and Bypass. Path graph and OpenGL come later; `juce_opengl` is already in the jucer.

Message thread:

- The editor scales a fixed base size the same way sibling plugins do.
- Host automatable surface is globals + six lenses + five crossovers. Motion and Wiring stay out of APVTS.

## Files

- `src/dsp/Names.h` — choice labels and counts. JUCE-free for `just check`.
- `src/parameters/ParameterIDs.h` — APVTS string IDs. Do not rename.
- `src/parameters/ParameterLayout.h` — the host surface.
- `src/ui/Theme.h` — Forest Depth, six hexes.
