# PrLogueXXL sources (shared by build.sh and the tests)
SDK=third_party/logue-sdk
PLUGIN_SRCS="src/engine.c src/synth.c src/voice.c src/panel.c src/program.c src/fx.c"
USER_SRCS="src/user.cpp $SDK/waves/waves.cpp"
TABLE_SRCS="$SDK/websim-dsp/waves-a.c $SDK/websim-dsp/waves-b.c $SDK/websim-dsp/waves-c.c $SDK/websim-dsp/waves-d.c $SDK/websim-dsp/waves-e.c $SDK/websim-dsp/waves-f.c $SDK/websim-dsp/bitres_lut.c $SDK/websim-dsp/midi_to_hz_lut.c $SDK/websim-dsp/tanpi_lut.c src/logue_shim/osc_white.c"
INCS="-Isrc -Ibuild -Isrc/logue_shim -I$SDK/prologue-inc -I$SDK/prologue-inc/utils -I$SDK/prologue-inc/dsp -I$SDK/websim-dsp"
USER_DEFS="-D_hook_init=waves_hook_init -D_hook_cycle=waves_hook_cycle -D_hook_on=waves_hook_on -D_hook_off=waves_hook_off -D_hook_mute=waves_hook_mute -D_hook_value=waves_hook_value -D_hook_param=waves_hook_param"
