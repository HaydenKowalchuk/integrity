# GameJam - Meson Build System

This project now uses Meson as its primary build system, replacing the complex makefile setup.

## Quick Start

```bash
# Cross-compile for Mac
meson setup build-linux --buildtype=debug -Dplatform=macos
meson compile -C build-macos

# Cross-compile for Linux
meson setup build-linux --buildtype=release -Dplatform=linux --cross-file x86_64-linux
meson compile -C build-linux

# Cross-compile for Dreamcast
meson setup build-dreamcast --buildtype=release -Dplatform=dreamcast --cross-file sh4-dreamcast-kos
meson compile -C build-dreamcast

# Cross-compile for PSP
meson setup build-psp --buildtype=release -Dplatform=psp --cross-file mips-allegrex-psp
meson compile -C build-psp

# Cross-compile for Windows
meson setup build-windows --buildtype=release -Dplatform=windows --cross-file x86_64-mingw32
meson compile -C build-windows

# Clean
rm -rf build-*
```

## Supported Platforms

- **Linux**: Uses GLFW, GLAD, OpenAL
- **macOS (Apple Silicon)**: Uses GLFW, GLAD, OpenAL via Homebrew
- **Windows**: Uses GLFW, GLAD, OpenAL  
- **Dreamcast**: Uses libGLdc, KOS toolchain
- **PSP**: Uses PSP GU, PSPSDK

## Platform-specific Requirements

### Linux
```bash
# Ubuntu/Debian
sudo apt install libglfw3-dev libgl1-mesa-dev libopenal-dev

# Or use system packages
```

### macOS (Apple Silicon)
```bash
# Install dependencies via Homebrew
brew install glfw pkg-config openal-soft

# For meson
brew install meson ninja
```

### Windows
- MinGW-w64 toolchain
- GLFW3, GLAD, OpenAL libraries

### Dreamcast
- KOS toolchain: `/opt/toolchains/dc/`
- libGLdc dependency
- Docker alternative: `einsteinx2/dcdev-kos-toolchain`

### PSP
- PSPSDK toolchain: `/usr/local/pspdev/`
- Docker alternative: `sharkwouter/pspdev`

## Examples

All examples in the `examples/` directory are automatically built when you run the main build. Each example becomes a separate executable.

## Migration from Makefiles

The old makefile system is still available for reference, but Meson is now the recommended build system:

- Faster builds and better dependency tracking
- Cross-compilation support
- Cleaner configuration
- Better integration with IDEs and editors

## Build Options

- `platform`: Target platform (linux, windows, dreamcast, psp)
- `buildtype`: Debug or release build

## Directory Structure

```
build-linux/     # Linux build output
build-windows/   # Windows build output  
build-dreamcast/ # Dreamcast build output
build-psp/       # PSP build output
```
