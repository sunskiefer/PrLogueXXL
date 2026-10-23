#!/usr/bin/env bash
# Build PrLogueXXL for MPC OS (armv7-a hard-float, glibc <= 2.31 so it also loads on MPC OS 2.x).
#   ./build.sh                 (MPC_VST=../mpc-vst-plugins to use another framework checkout)
# Needs: python3 + Pillow + numpy, Playwright's Chromium (the skin's browser renderer) and Zig (pip install ziglang).
# Output: build/package/L'Cronx - VST - PrLogueXXL/  (skin + prloguexxl.so)
set -euo pipefail
cd "$(dirname "$0")"
MPC_VST="${MPC_VST:-$PWD/third_party/mpc-vst-plugins}"
ZIG="${ZIG:-python3 -m ziglang}"
NAME="L'Cronx - VST - PrLogueXXL"
. ./build_srcs.sh
python3 tools/gen_params.py
python3 tools/panel.py

rm -rf build/skin
python3 "$MPC_VST/tools/gen_vst.py" vst.json 2>&1 | grep -v -e "TOUCH" -e "EDGE" -e "MODULE_DIR" || true
[ -f build/params.h ] && [ -d "build/skin/$NAME/Plugin Skins" ]
python3 "$MPC_VST/tools/skin_check.py" "build/skin/$NAME"      # touch boxes, edges, Q-Links, as MPC loads it

TGT="-target arm-linux-gnueabihf.2.31 -mcpu=generic+v7a+vfp3d16-d32-neon+thumb2 -ffp-contract=off"
COMMON="-D_FILE_OFFSET_BITS=64 -O2 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections -DNDEBUG -w"
mkdir -p build/arm
OBJS=""
for f in $PLUGIN_SRCS src/wrapper/vst2_wrap.c $TABLE_SRCS; do
  o="build/arm/$(echo "$f" | tr / _).o"
  $ZIG cc $TGT $COMMON -std=gnu11 -Ibuild -I"$MPC_VST/wrapper" $INCS -c "$f" -o "$o"
  OBJS="$OBJS $o"
done
for f in $USER_SRCS; do
  o="build/arm/$(echo "$f" | tr / _).o"
  $ZIG c++ $TGT $COMMON -std=gnu++14 -fno-exceptions -fno-rtti $INCS $USER_DEFS -c "$f" -o "$o"
  OBJS="$OBJS $o"
done
printf '{\n  global: VSTPluginMain;\n  local: *;\n};\n' > build/arm/exports.map
$ZIG cc $TGT -shared -fPIC -Wl,--no-undefined -Wl,--version-script=build/arm/exports.map -Wl,--gc-sections -Wl,-s \
  $OBJS -lm -lpthread -o build/arm/prloguexxl.so
PKG="build/package/$NAME"
rm -rf build/package && mkdir -p "$PKG"
cp -r "build/skin/$NAME/." "$PKG/"
cp build/arm/prloguexxl.so "$PKG/"
mkdir -p "$PKG/licenses"
cp third_party/logue-sdk/LICENSE "$PKG/licenses/logue-sdk-LICENSE.txt"
cp art/fonts/OFL-Barlow.txt "$PKG/licenses/" 2>/dev/null || true
echo "-> $PKG"
