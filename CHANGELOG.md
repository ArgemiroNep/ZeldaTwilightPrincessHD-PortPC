# Changelog

Twilight Princess HD for Windows, Linux & Android. Each version is a git tag (`vX.Y.Z`) and a GitHub Release.

## v0.1.0 — first release (Windows, Linux, Android)

The starting point of this repository: native Windows and Linux builds of the Twilight Princess HD port,
packaged with a first-start setup that builds the game from the player's own dump, and an Android app built
by the player from their own game.

**Windows, Linux and Android**
- Native Windows (x86-64) build with Vulkan and SDL3, from the macOS port by AsadSi.
- Native Linux build (x86-64; arm64 once the repository is public), glibc 2.35 or newer: unzip and start
  `twilight-princess-hd`; the setup uses your Python 3 and a pinned zig compiler. Also builds with GCC's
  standard library (two fixes for libstdc++).
- Android (arm64, Android 13+, Vulkan 1.3): an SDL3 app (`android/`) that the player builds on a computer
  from their own game code (`android/build_native.sh`, Gradle). No APK is ever published.
- First-start setup (`Twilight Princess HD.exe`): asks for the game folder, Cemu archive or disc image, checks
  the version, translates and compiles the game code locally and starts the game; later starts launch it
  directly (hold Shift for the setup). Portable: everything stays in the release folder.
- Release packaging and CI for Windows and Linux, and an Android build check (`tools/release`,
  `.github/workflows/release.yml`), with a guard that fails the build if a package contains game code, game
  files or keys. Windows programs declare UTF-8 and long paths in their manifest.

**Display and performance**
- 60 fps frame interpolation by default (120 / 240 selectable), 2560x1440 internal resolution.
- The dialogue's letter halos animate at the game's speed at any frame rate (no flicker), HUD panes drawn
  several times per frame are not interpolated, pulsing glows keep their place.
- The picture matches the window's shape (no black side bars).

**Controls**
- The Wii U Pro Controller is selected automatically (the game's connect callback is called as on hardware).
- Any SDL3 controller; buttons keep the Wii U Pro Controller's positions.
- Button prompts follow the controller (automatic, or fixed in *F1 > Controls*):
  - Xbox: B / A / Y / X in place of the Wii U letters (the action button's highlight stays on the right
    button), LB / RB / LT / RT, in the HUD, the item screen, the file screens and the action prompts.
  - PlayStation: circle / cross / triangle / square, L1 / R1 / L2 / R2.
  - Nintendo controllers and keyboard: the original icons.
  The captions are rebuilt at run time from the game's own button textures; no artwork is shipped.
- "Skip" stays on + (Start).

**Other**
- Spanish (Latin America) by default; the game's other languages in *F1 > Language*.
- Audio tab (volume, mute), Quit button, settings saved per user.
- Log file `tphd.log`; diagnostics for button prompts with `WWHD_LOG_GLYPHS=1`.
