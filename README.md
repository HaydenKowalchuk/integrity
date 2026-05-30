# Integrity Engine Framework
## Free and Open for the Community!

![logo](/images/Integrity_XL_trans_black.png?raw=true)

A cross-platform game engine written in **C99** and built with **Meson**.

## Working:
- 2D Rendering
- 3D Rendering
- Cross Platform: **macOS**, **Windows**, **Linux**, **Dreamcast**, **PSP**
- Sound
- Controller Input
- Scene Switching
- Smart Asset Handling
- Abstracted File IO

# Quick Start

## Prerequisites
- [Meson](https://mesonbuild.com) >= 0.65.0
- [Ninja](https://ninja-build.org)
- Platform-specific dependencies (see below)

## Clone
```bash
git clone https://github.com/HaydenKow/integrity.git
cd integrity
```

## Build

### Native Build (macOS / Linux)
```bash
meson setup builddir --buildtype=release
meson compile -C builddir
```

### Cross-Compilation
```bash
# Linux
meson setup build-linux --buildtype=release -Dplatform=linux

# Windows
meson setup build-windows --buildtype=release -Dplatform=windows --cross-file x86_64-mingw32

# Dreamcast
meson setup build-dreamcast --buildtype=release -Dplatform=dreamcast --cross-file sh4-dreamcast-kos

# PSP
meson setup build-psp --buildtype=release -Dplatform=psp --cross-file mips-allegrex-psp
```

# Platform-Specific Requirements

## macOS (Apple Silicon / Intel)
```bash
brew install meson ninja
meson setup build-macos --buildtype=release
meson compile -C build-macos
```

## Linux
```bash
# Ubuntu/Debian
sudo apt install libgl1-mesa-dev meson ninja-build
meson setup build-linux --buildtype=release -Dplatform=linux
meson compile -C build-linux
```

## Windows (MinGW)
```bash
# Install MinGW-w64 toolchain, then:
meson setup build-windows --buildtype=release -Dplatform=windows --cross-file x86_64-mingw32
meson compile -C build-windows
```

## Dreamcast
Requires KOS toolchain at `/opt/toolchains/dc/`. Docker alternative: `einsteinx2/dcdev-kos-toolchain`.
```bash
meson setup build-dreamcast --buildtype=release -Dplatform=dreamcast --cross-file sh4-dreamcast-kos
meson compile -C build-dreamcast
```

## PSP
Requires PSPSDK at `/usr/local/pspdev/`. Docker alternative: `sharkwouter/pspdev`.
```bash
meson setup build-psp --buildtype=release -Dplatform=psp --cross-file mips-allegrex-psp
meson compile -C build-psp
```

# Build Options

| Option       | Values                                           | Default  | Description                  |
|--------------|--------------------------------------------------|----------|------------------------------|
| `platform`   | `macos`, `linux`, `windows`, `dreamcast`, `psp`  | `macos`  | Target platform              |
| `buildtype`  | `debug`, `release`, `custom`                     | `debug`  | Optimization and debug info  |

### Clean
```bash
rm -rf build-*
```

## Cross Files
Pre-configured cross-compilation files are provided at the project root:

| File                   | Target            |
|------------------------|-------------------|
| `sh4-dreamcast-kos`    | Sega Dreamcast    |
| `mips-allegrex-psp`    | Sony PSP          |

For Linux and Windows cross-builds, provide your own cross file or use a native toolchain.

# Directory Structure

| Path | Description |
|------|-------------|
| `include/` | Public API headers |
| `src/common/` | Platform-independent engine code |
| `src/macos/` | macOS backend (GLAD, sys_macos, sys_sound) |
| `src/linux/` | Linux backend (GLAD, sys_linux, sys_sound) |
| `src/windows/` | Windows backend (GLAD, sys_windows, sys_sound) |
| `src/dreamcast/` | Dreamcast backend (perfctr, sys_dreamcast, sys_sound) |
| `src/psp/` | PSP backend (sys_psp, sys_sound) |
| `src/scene/` | Scene graph / state management |
| `src/ui/` | UI rendering (GL batcher, backend) |
| `src/cooker/` | Asset processing tool |
| `examples/hello/` | Hello World scene example |
| `examples/input_test/` | Controller input test example |
| `third-party/` | Vendored libraries (glad, cute_c2, tinyobj_loader) |
| `subprojects/` | Meson wrap dependencies (glfw, openal-soft, cglm, stb, umka) |
| `sh4-dreamcast-kos` | Dreamcast cross-compilation file |
| `mips-allegrex-psp` | PSP cross-compilation file |

## Dependencies

### Vendored (third-party/)
- `glad` — OpenGL loader
- `cute_c2` — 2D collision detection
- `tinyobj_loader` — OBJ mesh loader

### Meson Subprojects (subprojects/)
- `glfw` — Windowing and input
- `openal-soft` — Audio
- `cglm` — OpenGL math library
- `stb` — Image loading (macOS only)
- `umka` — Scripting language

## Output

The build produces `libintegrity.a` (static library) and the `cooker` asset processing tool.

## Examples

Two example programs are built alongside the engine:

| Example       | Description                          | Run Target           |
|---------------|--------------------------------------|----------------------|
| `hello`       | Basic scene with Hello World text    | `meson compile -C builddir run_hello` |
| `input_test`  | Controller input diagnostic display  | `meson compile -C builddir run_input_test` |

Build and run an example in one step:
```bash
meson compile -C builddir run_hello
meson compile -C builddir run_input_test
```
