<img src="assets/logo.png" alt="fD1ST" width="160">

# fD1ST

**fD1ST** is DIST, a stereo distortion / saturation effect. Spoken name DIST. The leading *f* is fx. Never “fist.” Tagline: **Watch the signal break.**

One through Cavity, 34 styles, Path/Focus/Motion, 32× Superb, linear-phase OS, 48 factory programs. Work in progress for the catalog. The logo in `assets/logo.png` is a placeholder until the Meat Bag drops the Affinity export.

`just self-test` runs the standalone with `--self-test`.

VST3. Stupid Audio. [MIT](LICENSE). Site: [docs/](docs/index.html). Manual: [docs/docs.html](docs/docs.html).

The site backdrop is `docs/fx.js`, a PixiJS scene kept in sync with the other Stupid Audio sites. Add `?fx` to any page to preview it full screen with the page chrome hidden; `?fx=false` keeps the normal page.

## Release

Windows, Linux, and macOS files come from GitHub Actions (`.github/workflows/dist.yml`). Run it from the Actions tab for artifacts only, or push a numeric tag such as `0.0.1` (no `v` prefix) to build and publish a GitHub Release with the files attached. `<version>` is the `version` in `fD1ST.jucer`.

The `dist` artifact holds:

- `fD1ST-<version>-win-x64-vst3.zip`
- `fD1ST-<version>-win-x64-installer.msi`
- `fD1ST-<version>-linux-x64-vst3.zip`
- `fD1ST-<version>-mac-universal-vst3.zip`

The MSI installs the VST3 into `Common Files\VST3` by default; the folder can be changed.

The Mac zip is ad-hoc signed, not notarized. Gatekeeper blocks it until you right-click Open, or run `xattr -dr com.apple.quarantine` on the `.vst3`. The Linux build uses the system ALSA, FreeType, fontconfig, and X11 libraries.

`just build-dist` on Windows writes the VST3 zip and the MSI. Building the MSI needs the .NET 8 SDK.

`just docs-screenshots` overwrites `docs/Screenshot1.png` and `docs/Screenshot2.png`. Needs a debug standalone (`just build-debug`) and an interactive desktop.
