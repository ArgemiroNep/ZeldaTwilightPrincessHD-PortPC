# Credits

- **[ZeldaWWHDRecomp](https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp)**, the Wind Waker HD port the
  Twilight Princess HD ports are forked from: the recompiler, Cafe OS layer, renderers, setup, release tooling
  (`tools/release/package.py`, `tools/release/guard.py` and the release workflow are adapted from it) and almost
  everything else; the Android app (`android/`) and its build script are adapted from it too, after the
  Android port by rhemfur. Lukas S (lsch) and contributors: rhemfur (Rhener Furtado), Sean13128, Eduardo Serralvo Gil,
  Manoah B., depende3000, fda, arcadematicas, mhbxyz. MPL-2.0.
- **[AsadSi/twilight-princess-hd-recomp](https://github.com/AsadSi/twilight-princess-hd-recomp)**, the Twilight
  Princess HD port (macOS) this port starts from. MPL-2.0.
- **[Cemu](https://github.com/cemu-project/Cemu)**, whose documentation of the Wii U GPU and GX2 the project
  follows, and whose address library and shader decompiler are vendored in `runtime/third_party/cemu`. MPL-2.0.
- **[zeldaret/tp](https://github.com/zeldaret/tp)**, the Twilight Princess decompilation: names and structures
  used throughout.
- Third-party libraries and tools: SDL3 (also its Android Java files), Dear ImGui, fmt, xxHash, glslang, zlib,
  LZ4, Zstandard, metal-cpp, the llvm-mingw toolchain (Windows), Zig (Linux), the embeddable Python shipped
  with the Windows release, the Android SDK, NDK and Gradle. Their licenses are in
  `runtime/third_party` and, in a release, in `third-party-licenses`.
- **GitHub**, for hosting and for the Actions runners that build the releases.

## How this was made

The Twilight Princess HD additions and the Windows, Linux and Android work in this repository (Vulkan on
Windows and Linux, the Android app, frame pacing, controller selection, button prompts for Xbox /
PlayStation / Nintendo controllers, settings, release packaging) were developed with AI assistance:
**Claude** (Anthropic), **Codex** (OpenAI) and **DeepSeek**. Bugs in those additions are ours, not the
original authors'.
