#!/bin/sh
# PSP: miniaudio explicitly sets pic:true on static_library targets,
# but the PSP toolchain (mips-allegrex-eabi) cannot generate PIC with -mabi=eabi.
# Override to pic:false since PSP links everything statically.
#
# Everything else (backends, runtime_linking, etc.) is configured
# via default_options in the root meson.build dependency() call.

MINIAUDIO_DIR="subprojects/miniaudio-0.11.22"

if [ ! -f "$MINIAUDIO_DIR/meson.build" ]; then
  echo "miniaudio subproject not found at $MINIAUDIO_DIR"
  echo "Run 'meson setup build-psp -Dplatform=psp' first."
  exit 1
fi

sed -i '' 's/  pic: true,/  pic: false,/' "$MINIAUDIO_DIR/meson.build"
sed -i '' 's/      pic: true,/      pic: false,/' "$MINIAUDIO_DIR/extras/nodes/meson.build"

echo "Patched miniaudio meson.build files for PSP (pic: true -> pic: false)"
