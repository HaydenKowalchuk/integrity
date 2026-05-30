# Integrity Engine — Meson Build System

This project uses **Meson** as its primary build system with **Ninja** as the backend. The old Makefile system is deprecated.

## Supported Platforms

| Platform    | Graphics       | Audio   | Toolchain                        |
|-------------|----------------|---------|----------------------------------|
| macOS       | GLFW + GLAD    | OpenAL  | Native (Clang)                   |
| Linux       | GLFW + GLAD    | OpenAL  | Native (GCC/Clang)               |
| Windows     | GLFW + GLAD    | OpenAL  | MinGW-w64 (cross)                |
| Dreamcast   | libGLdc        | OpenAL  | KOS `sh-elf-gcc` (cross)         |
| PSP         | PSP GU         | PSP SDK | `psp-gcc` (cross)                |

## Quick Start

### macOS (native, default platform)
```bash
brew install meson ninja
meson setup build --buildtype=release
meson compile -C build
```

### Cross-Compilation for Other Platforms
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

## Build Options

Configured in `meson_options.txt`:

| Option       | Values                                           | Default  | Description                  |
|--------------|--------------------------------------------------|----------|------------------------------|
| `platform`   | `macos`, `linux`, `windows`, `dreamcast`, `psp`  | `macos`  | Target platform              |
| `buildtype`  | `debug`, `release`, `custom`                     | `debug`  | Optimization and debug info  |

### Debug vs Release

- **debug**: `-DDEBUG -g -O0 -fno-inline`
- **release**: `-DNDEBUG -DQUIET -Os -ffast-math -fomit-frame-pointer`

## Directory Layout

```
.
├── include/            # Public API headers
├── src/
│   ├── common/         # Platform-independent engine code
│   ├── macos/          # macOS backend (GLAD, sys_macos, sys_sound)
│   ├── linux/          # Linux backend (GLAD, sys_linux, sys_sound)
│   ├── windows/        # Windows backend (GLAD, sys_windows, sys_sound)
│   ├── dreamcast/      # Dreamcast backend (perfctr, sys_dreamcast, sys_sound)
│   ├── psp/            # PSP backend (sys_psp, sys_sound)
│   ├── scene/          # Scene graph / state management
│   ├── ui/             # UI rendering (GL batcher, backend)
│   └── cooker/         # Asset processing tool
├── third-party/        # Vendored libraries (glad, cute_c2, tinyobj_loader)
├── subprojects/        # Meson wrap dependencies (glfw, openal-soft, cglm, stb, umka)
├── sh4-dreamcast-kos   # Dreamcast cross-compilation file
└── mips-allegrex-psp   # PSP cross-compilation file
```

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

The build produces `libgamejam.a` (static library) and the `cooker` asset processing tool. Example binaries are built from the `examples/` directory when present.
