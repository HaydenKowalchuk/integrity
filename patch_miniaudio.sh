#!/bin/sh
# PSP: miniaudio explicitly sets pic:true on static_library targets,
# but the PSP toolchain (mips-allegrex-eabi) cannot generate PIC with -mabi=eabi.
# Override to pic:false since PSP links everything statically.
#
# Also adds 'psp' to the default_backends dict so that backends=custom,null
# are resolved when cross-compiling for PSP with runtime_linking=disabled.
#
# Also patches miniaudio.h to add PSP backend support (platform detection,
# vtable declarations, config struct fields, switch cases, stock backends).
#
# All other miniaudio options are configured via default_options
# in the root meson.build dependency() call.

MINIAUDIO_DIR="subprojects/miniaudio"
MINIAUDIO_H="$MINIAUDIO_DIR/miniaudio.h"

if [ ! -f "$MINIAUDIO_DIR/meson.build" ]; then
  echo "miniaudio subproject not found at $MINIAUDIO_DIR"
  echo "Run 'meson setup build-psp -Dplatform=psp' first."
  exit 1
fi

# Patch pic:true -> pic:false
sed -i '' 's/  pic: true,/  pic: false,/' "$MINIAUDIO_DIR/meson.build"
sed -i '' 's/      pic: true,/      pic: false,/' "$MINIAUDIO_DIR/extras/nodes/meson.build"

# Add 'psp' to default_backends so backends are resolved at config time
if ! grep -q "'psp': \[\]" "$MINIAUDIO_DIR/meson.build"; then
  sed -i '' "s/'emscripten': \['webaudio'\],/'emscripten': ['webaudio'],\n      'psp': [],/" "$MINIAUDIO_DIR/meson.build"
fi

# --- miniaudio.h patches ---
add_after_line() {
  local file="$1" pattern="$2" ins="$3"
  if grep -qF "$ins" "$file"; then
    echo "  [SKIP] already present: $ins"
    return
  fi
  local lineno
  lineno=$(grep -n "$pattern" "$file" | tail -1 | cut -d: -f1)
  if [ -z "$lineno" ]; then
    echo "  [FAIL] pattern not found: $pattern"
    return
  fi
  sed -i '' "${lineno}a\\
$ins" "$file"
  echo "  [OK] inserted after line $lineno"
}

echo "Patching miniaudio.h for PSP..."

# 1) Platform detection: add MA_PSP after MA_VITA
add_after_line "$MINIAUDIO_H" \
  "#define MA_VITA.*Vita SDK" \
  "    #if defined(__psp__)"

lineno=$(grep -n "#define MA_VITA.*Vita SDK" "$MINIAUDIO_H" | tail -1 | cut -d: -f1)
lineno=$((lineno + 1))
if ! grep -q "#define MA_PSP" "$MINIAUDIO_H" 2>/dev/null; then
  sed -i '' "${lineno}a\\
        #define MA_PSP          \/\* Assuming PSP SDK. \*\\
    #endif" "$MINIAUDIO_H"
fi

# 2) Backend support: add MA_SUPPORT_PSP after MA_SUPPORT_DREAMCAST
if ! grep -q "MA_SUPPORT_PSP" "$MINIAUDIO_H" 2>/dev/null; then
  sed -i '' "/#define MA_SUPPORT_DREAMCAST/a\\
#if defined(MA_PSP)\\
    #define MA_SUPPORT_PSP\\
#endif" "$MINIAUDIO_H"
fi

# 3) Header declarations: add PSP section after Vita section end
if ! grep -q "miniaudio_psp.h" "$MINIAUDIO_H" 2>/dev/null; then
  sed -i '' "/END miniaudio_vita.h/a\\

\/\* BEG miniaudio_psp.h \*\/\\
extern ma_device_backend_vtable* ma_device_backend_psp;\\
MA_API ma_device_backend_vtable* ma_psp_get_vtable(void);\\
\\
\\
typedef struct\\
{\\
    int _unused;\\
} ma_context_config_psp;\\
\\
MA_API ma_context_config_psp ma_context_config_psp_init(void);\\
\\
\\
typedef struct\\
{\\
    int _unused;\\
} ma_device_config_psp;\\
\\
MA_API ma_device_config_psp ma_device_config_psp_init(void);\\
\/\* END miniaudio_psp.h \*\/" "$MINIAUDIO_H"
fi

# 4) Device config struct field
if ! grep -q "ma_device_config_psp" "$MINIAUDIO_H" 2>/dev/null; then
  sed -i '' "/ma_device_config_vita.*vita;/a\\
    ma_device_config_psp        psp;" "$MINIAUDIO_H"
fi

# 5) Context config struct field
if ! grep -q "ma_context_config_psp" "$MINIAUDIO_H" 2>/dev/null; then
  sed -i '' "/ma_context_config_vita.*vita;/a\\
    ma_context_config_psp       psp;" "$MINIAUDIO_H"
fi

# 6) Context config switch case
if ! grep -q "ma_device_backend_psp" "$MINIAUDIO_H" 2>/dev/null; then
  sed -i '' "/if (pVTable == ma_device_backend_vita)/a\\
        if (pVTable == ma_device_backend_psp) {\/
            return \&pConfig->psp;\\
        }" "$MINIAUDIO_H"
fi

# 7) Stock backends entry
if ! grep -q "ma_device_backend_psp.*NULL" "$MINIAUDIO_H" 2>/dev/null; then
  sed -i '' "/ma_device_backend_vita,.*NULL)/a\\
    if (backendsCap > count) { pBackends[count++] = ma_device_backend_config_init(ma_device_backend_psp,        NULL); }" "$MINIAUDIO_H"
fi

# 8) Device config switch case
if ! grep -q "psp" "$MINIAUDIO_H" 2>/dev/null || [ "$(grep -c "if (pVTable == ma_device_backend_psp)" "$MINIAUDIO_H")" -lt 2 ]; then
  # Find the second occurrence of vita device config switch and add after it
  sed -i '' "/if (pVTable == ma_device_backend_vita) {/{
    /return &pConfig->vita;/a\\
        if (pVTable == ma_device_backend_psp) {\\
            return &pConfig->psp;\\
        }
  }" "$MINIAUDIO_H"
fi

echo "miniaudio.h patches applied."
