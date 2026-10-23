#!/usr/bin/env bash
# PrLogueXXL offline tests: every factory program rendered (x86, ASan), the framework's VST host test (x86, ASan),
# and the built armv7 plugin loaded as MPC loads it, under qemu-arm.
# Known host_test notes: MOD EFFECT's SELECT springs back to ON (KORG behaviour), the MULTI TYPE knob stops at 4 for
# NOISE, and 10-bit knobs move ~10 per data-wheel click; host_test reports those as FAIL by its generic rules.
set -uo pipefail
cd "$(dirname "$0")/.."
. ./build_srcs.sh
export ASAN_OPTIONS=detect_leaks=0
SAN="-O1 -g -fsanitize=address,undefined -w -D_FILE_OFFSET_BITS=64"
mkdir -p build/test/obj
for f in $USER_SRCS; do g++ $SAN -std=gnu++14 $INCS $USER_DEFS -c $f -o build/test/obj/$(basename $f).o; done
for f in $TABLE_SRCS; do gcc $SAN $INCS -c $f -o build/test/obj/$(basename $f).o; done
gcc $SAN -std=gnu11 $INCS -Ithird_party/mpc-vst-plugins/wrapper $PLUGIN_SRCS test/render_test.c build/test/obj/*.o \
    -lstdc++ -lm -o build/test/render_test && ./build/test/render_test build/test | tail -3
python3 third_party/mpc-vst-plugins/tools/gen_vst.py vst.json --params-h >/dev/null
gcc $SAN -std=gnu11 -Ibuild -Ithird_party/mpc-vst-plugins/wrapper $INCS third_party/mpc-vst-plugins/tools/host_test.c \
    $PLUGIN_SRCS src/wrapper/vst2_wrap.c build/test/obj/*.o -lstdc++ -lm -lpthread -ldl -o build/test/host_test && \
    (cd build/test && ./host_test | grep -c '^ok' | sed 's/^/host_test: ok checks: /'; ./host_test | grep '^FAIL')
SO="build/package/L'Cronx - VST - PrLogueXXL/prloguexxl.so"
if command -v qemu-arm >/dev/null && [ -f "$SO" ]; then
  python3 -m ziglang cc -target arm-linux-gnueabihf.2.31 -mcpu=generic+v7a+vfp3d16-d32-neon+thumb2 -D_FILE_OFFSET_BITS=64 \
      -O1 -w test/arm_vst_host.c -ldl -lm -o build/test/arm_vst_host
  qemu-arm -L /usr/arm-linux-gnueabihf build/test/arm_vst_host "$SO" build/test
fi
