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
git clone --recursive https://github.com/HaydenKow/integrity.git
cd gamejam
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
| Option     | Values                                           | Default | Description           |
|------------|--------------------------------------------------|---------|-----------------------|
| `platform` | `macos`, `linux`, `windows`, `dreamcast`, `psp`  | `macos` | Target platform       |
| `buildtype`| `debug`, `release`, `custom`                     | `debug` | Build type            |

# Directory Structure
```
build-macos/        # macOS build output
include/            # Public headers
src/common/         # Shared engine code
src/macos/          # macOS platform layer
src/linux/          # Linux platform layer
src/windows/        # Windows platform layer
src/dreamcast/      # Dreamcast platform layer
src/psp/            # PSP platform layer
src/scene/          # Scene management
src/ui/             # UI rendering
src/cooker/         # Asset processing tool
third-party/        # Vendored dependencies
subprojects/        # Meson subproject dependencies
```
