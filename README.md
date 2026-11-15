# Twilight Princess HD for Windows, Linux & Android (Native Port)

An unofficial, native port of *The Legend of Zelda: Twilight Princess HD* (Wii U, USA) for **Windows**,
**Linux** and **Android**, built by static recompilation. The game's PowerPC code is translated to C on your
own machine, the Wii U system libraries the game uses are reimplemented natively, and the graphics run on
**Vulkan**. There is no emulator and no Cemu runtime involved.

This project continues [AsadSi/twilight-princess-hd-recomp](https://github.com/AsadSi/twilight-princess-hd-recomp)
(a macOS port), which is itself built on [ZeldaWWHDRecomp](https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp),
the Wind Waker HD port. See [Credits](#credits).

## Legal notice

This is a fan project. It is not affiliated with, endorsed or sponsored by Nintendo. *The Legend of Zelda*,
*Twilight Princess* and *Wii U* are trademarks of their respective owners and are used only to describe what
this software works with.

**This repository and its releases contain no game code, no game data and no keys.** You need your own,
legally obtained copy of the game. Everything game-specific (the extracted files, the recompiled code, shader
caches, an Android APK built from your game) is made on your machine from your dump and must not be shared.
Do not ask for, and do not share, game files or keys.

## What you need

- Your own dump of **Twilight Princess HD, USA** (title `00050000-1019E500`, version 81), as one of:
  - an extracted game folder (`code`, `content`, `meta`), for example from Cemu;
  - a Cemu archive (`.wua`);
  - a disc image (`.wux` / `.wud`) together with its disc key and the Wii U common key.
- A GPU with **Vulkan 1.3** drivers (or Vulkan 1.1 / 1.2 with `VK_KHR_dynamic_rendering`).
- About 10 GB of free space, and an internet connection on the first start (the setup downloads a pinned
  compiler once to build the game code on your computer).

| System | Download | Requirements |
|---|---|---|
| Windows | `TwilightPrincessHD-<version>-windows-x86_64.zip` | Windows 10 (1903+) or 11, 64-bit |
| Linux | `TwilightPrincessHD-<version>-linux-x86_64.zip` (or `linux-aarch64`) | glibc 2.35+ (Ubuntu 22.04+, Debian 12+, Fedora 36+, Arch, SteamOS 3), Python 3 |
| Android | built by you, see [Android](#android-build-it-yourself) | arm64, Android 13+, Vulkan 1.3 |

## Install and play

1. Download the zip for your system from the [Releases](../../releases) page and unzip it anywhere (for
   example `C:\Games\TwilightPrincessHD` or `~/Games/TwilightPrincessHD`).
2. Start the game:
   - **Windows:** `Twilight Princess HD.exe`. The release is not code-signed, so SmartScreen may say
     "Windows protected your PC": choose *More info > Run anyway*. Python comes with the release.
   - **Linux:** `twilight-princess-hd` (or `Twilight Princess HD.desktop`; some desktops ask to allow
     launching it first). Take the zip that matches `uname -m` (`x86_64` or `aarch64`). It uses your
     Python 3.
3. The first start opens the setup: choose your game folder, `.wua` or disc image. The setup checks that it
   is the supported version, translates the game's code to C, compiles it with a pinned compiler
   (llvm-mingw on Windows, zig on Linux, downloaded once into the folder and checked by SHA-256) and links
   it with the runtime. This takes a few minutes and happens only once.
4. Later starts launch the game directly. To repair, update or change the game, hold **Shift** while
   starting it (Windows) or start it with `--setup` (Linux; also the "Setup" action of its menu entry).

Everything stays inside the unzipped folder (`data/`): the built game, saves, settings and logs. To
uninstall, delete the folder.

## Android (build it yourself)

The Android version runs the same runtime through SDL3's Android activity, with the Vulkan renderer, on an
arm64 phone. Because the app can only exist together with the game code built from **your** game, there is
**no APK to download**: you build it on a computer from your own dump, and it is for your own phone only.
Never share an APK you built. (The CI builds the app with placeholder code only, to check that it compiles.)

You need:
- a phone with arm64, Android 13 or newer and Vulkan 1.3, and preferably a **game controller** (Bluetooth or
  USB);
- on the computer: the Android SDK (platform 36, build tools 35.0.0), NDK 30.0.16248370, JDK 17 or newer,
  CMake 3.20+, Ninja, Python 3 and Git Bash (Windows) or a Unix shell;
- your game code in `build/gen`, made from your own game (see [Build from source](#build-from-source-developers),
  step 2).

```sh
android/build_native.sh            # libmain.so, libSDL3.so, libc++_shared.so -> android/app/libs
#   CPU=generic (default): any arm64 phone. ANDROID_SDK, NDK_VERSION, GEN_DIR, JOBS: see the script.
python3 android/make_icon.py       # optional: the launcher icon from your game/meta/iconTex.tga (Pillow)
cd android && ./gradlew assembleRelease
adb install -r app/build/outputs/apk/release/app-release.apk
```

Start the app once: it creates `Android/data/io.github.argemironep.tphd/files/` on the phone's storage
(reachable over USB from a computer). Copy your extracted game there as `game/` (`code/`, `content/`,
`meta/`) and start the app again. `save/` holds the saves, `config/` the settings and shader caches, and an
optional `env.txt` takes `WWHD_` options, one `NAME=value` per line. The settings overlay opens by holding
Select / Minus on the controller. 60 fps is on by default and pauses itself when the phone is too slow or too
hot. Logs: `adb logcat -s wwhd`.

## Controls

- **Controllers:** any controller supported by SDL3 (Xbox, PlayStation, Switch Pro, generic XInput).
  Buttons keep the positions of the Wii U Pro Controller.
- **Button prompts follow your controller** (automatic by default, *F1 > Controls > Button prompts*):
  - Xbox: the game's icons show Xbox names (B / A / Y / X, LB / RB, LT / RT), with the action button's
    highlight on the right button as on the Wii U.
  - PlayStation: circle, cross, triangle and square, L1 / R1, L2 / R2.
  - Nintendo controllers and keyboard: the game's original icons.
- **Keyboard (defaults):** move WASD, camera arrow keys, A = K / Space, B = J, X = L, Y = I, L = Q, R = E,
  ZL = Left Shift, ZR = C, + = Enter, − = Tab, D-pad = 1–4. All bindings can be changed in *F1 > Controls*.
- **F1** (or holding Select / Minus on a controller) opens the settings overlay (graphics, display, audio,
  controls, language, quit).

## Features of this port

- Native Windows, Linux and Android builds with Vulkan and SDL3; 60 fps frame interpolation (also 120 / 240),
  with the dialogue and HUD animations kept at the right speed; 2560x1440 internal resolution by default on
  the PC.
- The picture matches the window's shape (no black side bars).
- Wii U Pro Controller selected automatically; the game's languages selectable in *F1 > Language*.
- Button prompts for Xbox, PlayStation and Nintendo controllers, drawn from the game's own button
  textures at run time (no third-party artwork is shipped).
- Audio volume and mute, quit button, settings saved per user.

The changes in each version are in [CHANGELOG.md](CHANGELOG.md).

## Build from source (developers)

The release workflow ([.github/workflows/release.yml](.github/workflows/release.yml)) is the reference build:
it compiles the runtime with the pinned toolchains (llvm-mingw on Windows, zig on Linux) against placeholder
guest code (`tools/recomp/stubgen.py`), packages it with `tools/release/package.py` and checks every package
with `tools/release/guard.py`. The game code is never part of a build made there.

To build and run the game yourself:

```sh
# 1. put your extracted game in game/ (game/code/Zelda.rpx, game/content, game/meta); it is ignored by git
# 2. translate Zelda.rpx to C (writes build/gen; this is game code: never commit or share it)
python3 tools/recomp/recomp.py game/code/Zelda.rpx build/gen
# 3. build (Linux: clang or gcc with the SDL3 build dependencies; Windows: MSYS2 CLANG64 shell)
cmake -S . -B build/rel -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ -DWWHD_BUNDLED_DEPS=ON
cmake --build build/rel --target wwhd
# 4. run from the folder that contains game/
./build/rel/wwhd
```

Windows packages for MSYS2 (CLANG64): `pacman -S --needed mingw-w64-clang-x86_64-{toolchain,cmake,ninja,python3,vulkan-headers,vulkan-loader}`.

Logs go to `tphd.log` next to the game (Windows) or the terminal (Linux). Useful switches:
`WWHD_LOG_GLYPHS=1` (button prompt diagnostics), `WWHD_INTERP=0` (30 fps). How the recompilation works:
[docs/how-it-works.md](docs/how-it-works.md).

## Credits

This port exists thanks to the work of others:

- **[ZeldaWWHDRecomp](https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp)** — Lukas S (lsch) and contributors:
  the recompiler, the Cafe OS layer, the Vulkan renderer, the Android port (rhemfur), the setup and release
  tooling and almost everything this port runs on (MPL-2.0).
- **[AsadSi/twilight-princess-hd-recomp](https://github.com/AsadSi/twilight-princess-hd-recomp)** — Asad: the
  Twilight Princess HD port this version starts from.
- **[Cemu](https://github.com/cemu-project/Cemu)** — the documentation of the Wii U GPU and GX2, and the address
  library and shader decompiler used by the renderer (MPL-2.0).
- **[zeldaret/tp](https://github.com/zeldaret/tp)** — the Twilight Princess decompilation, the source of the
  names and structures used throughout.
- **[SDL](https://github.com/libsdl-org/SDL)**, **[Dear ImGui](https://github.com/ocornut/imgui)**,
  **[fmt](https://github.com/fmtlib/fmt)**, **[xxHash](https://github.com/Cyan4973/xxHash)**,
  **[glslang](https://github.com/KhronosGroup/glslang)**, **[zlib](https://zlib.net/)**,
  **[LZ4](https://github.com/lz4/lz4)**, **[Zstandard](https://github.com/facebook/zstd)**,
  **[llvm-mingw](https://github.com/mstorsjo/llvm-mingw)**, **[Zig](https://ziglang.org/)**,
  **[Python](https://www.python.org/)** and the **Android SDK and NDK**.
- **[GitHub](https://github.com/)**, for hosting the code and building the releases.
- **AI assistance:** the work in this repository was developed with the help of **Claude** (Anthropic),
  **Codex** (OpenAI) and **DeepSeek**.

More details in [CREDITS.md](CREDITS.md).

## License

[Mozilla Public License 2.0](LICENSE), as the projects it is based on. Third-party components keep their own
licenses (listed in `third-party-licenses` in each release).
