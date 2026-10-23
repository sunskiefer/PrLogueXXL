/* PrLogueXXL -- the MULTI ENGINE's USER slots: logue SDK oscillators built from source into the plugin.
 *
 * Slot 1 = KORG's "waves" (logue-sdk platform/prologue/waves, BSD-3-Clause), using KORG's own wavetables A-F from the
 * logue-sdk websim (third_party/logue-sdk/websim-dsp). The prologue runs a user oscillator for each voice; here each
 * voice has its own copy of the oscillator's state (third_party/logue-sdk/waves/waves.cpp keeps one per voice).
 * Slots 2-16 are empty (silent), as on a prologue with no user oscillators installed. */
#include <stdint.h>
#include <string.h>
#include "userosc.h"

extern "C" {
int pl_uosc_voice = 0;
void waves_hook_init(uint32_t platform, uint32_t api);
void waves_hook_cycle(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames);
void waves_hook_on(const user_osc_param_t *const params);
void waves_hook_off(const user_osc_param_t *const params);
void waves_hook_param(uint16_t index, uint16_t value);

static int s_inited;
static uint16_t s_shape[16] = {0}, s_shift[16] = {0};

void user_osc_init(void)
{
    int v;
    if (s_inited) return;
    s_inited = 1;
    for (v = 0; v < 16; v++) { pl_uosc_voice = v; waves_hook_init(0, 0); }
}

void user_osc_param_all(int slot, int index, int value)
{
    int v;
    if (slot != 0) return;
    user_osc_init();
    for (v = 0; v < 16; v++) { pl_uosc_voice = v; waves_hook_param((uint16_t)index, (uint16_t)value); }
}

void user_osc_noteon(int slot, int voice, int note)
{
    user_osc_param_t p;
    if (slot != 0) return;
    user_osc_init();
    memset(&p, 0, sizeof p);
    p.pitch = (uint16_t)(((note < 0 ? 0 : note > 127 ? 127 : note) << 8));
    pl_uosc_voice = voice & 15;
    waves_hook_on(&p);
}

void user_osc_noteoff(int slot, int voice)
{
    user_osc_param_t p;
    if (slot != 0) return;
    memset(&p, 0, sizeof p);
    pl_uosc_voice = voice & 15;
    waves_hook_off(&p);
}

void user_osc_cycle(int slot, int voice, int note, float frac, float shape, float shift, float lfo, float *out, int n)
{
    user_osc_param_t p;
    int32_t y[64];
    int i;
    const int v = voice & 15;
    if (slot != 0 || n > 64) { for (i = 0; i < n; i++) out[i] = 0.f; return; }
    user_osc_init();
    pl_uosc_voice = v;
    {   /* SHAPE / SHIFT SHAPE as the panel sends them (10-bit) */
        const uint16_t sh = (uint16_t)(shape * 1023.f + 0.5f), ss = (uint16_t)(shift * 1023.f + 0.5f);
        if (sh != s_shape[v]) { s_shape[v] = sh; waves_hook_param(k_user_osc_param_shape, sh); }
        if (ss != s_shift[v]) { s_shift[v] = ss; waves_hook_param(k_user_osc_param_shiftshape, ss); }
    }
    memset(&p, 0, sizeof p);
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    p.pitch = (uint16_t)((note << 8) | (int)(frac * 255.f));
    p.shape_lfo = (int32_t)(lfo * 2147483647.f * 0.5f);
    waves_hook_cycle(&p, y, (uint32_t)n);
    for (i = 0; i < n; i++) out[i] = (float)y[i] * (1.f / 2147483648.f);
}
}
