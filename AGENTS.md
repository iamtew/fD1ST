# fD1ST – Cursor Rules

Spec: [docs/SPEC.md](docs/SPEC.md).

DIST distortion / saturation FX. Spoken name **DIST**. Never “fist”. VST3 + Standalone. Part of Stupid Audio; sister of fR3Z0 and fN0B.

**Rewrite in progress.** Step 0 stub only (stereo thru + Forest Depth). Not on the catalog until the Meat Bag asks. Do not rebuild the old Path / Focus / Motion product unless asked.

## Stack
- JUCE (Projucer workflow only – no CMake)
- C++17, real-time safe: no allocations, no locks, no system calls in processBlock
- APVTS for host parameters
- Just + justfile
- JUCE from `%USERPROFILE%\JUCE`
- Clanker never modifies `assets/logo.png` or `assets/logo.af`. Meat Bag owns the Affinity export; that PNG is source of truth. Only derived files: `just docs-logo` → `docs/logo.png`, `just icon` → `Builds/VisualStudio2022/icon.ico`.

## Layout
src/
 PluginProcessor.h/.cpp
 PluginEditor.h/.cpp
 parameters/
 ui/Theme.h
tests/fd1st_check.cpp
docs/
justfile
fD1ST.jucer

## Version
- Jucer `version=` is what the binary and About dialog show.
- Git tags must match that string (`0.0.1`).
- `just build-dist` names the zip and MSI from the jucer version.

## Colors
Forest Depth. Main **is** the plugin.

- #900020 Main (background, chrome, large surfaces)
- #0F6B4E Deep Emerald (accent, wet, selection)
- #A8C5B0 Soft Sage (text, dry ghost)
- #1A3C32 Dark Forest (wells, nested panels)
- #000000 Black (voids, plots)
- #FFFFFF White (peaks, readouts)

Ship only these six hexes. Mix and alpha of them is allowed.

## Do NOT
- Introduce CMake
- Allocate on the audio thread
- Copy commercial plugin code
- Copy sibling plugin source into this tree; read it, then write an fD1ST-sized version
- List this plugin on the Stupid Audio catalog until asked
