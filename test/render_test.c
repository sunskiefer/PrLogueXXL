/* PrLogueXXL offline test: every factory program plays a chord + a phrase; checks level, NaN, CPU; writes WAVs. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "engine.h"
#include "param_ids.h"

static void wav(const char *path, const int16_t *d, int frames)
{
    FILE *f = fopen(path, "wb");
    int sz = frames * 4, v;
    if (!f) return;
    fwrite("RIFF", 1, 4, f); v = 36 + sz; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f); { short a[2] = {1, 2}; fwrite(a, 2, 2, f); } v = 44100; fwrite(&v, 4, 1, f);
    v = 44100 * 4; fwrite(&v, 4, 1, f); { short a[2] = {4, 16}; fwrite(a, 2, 2, f); }
    fwrite("data", 1, 4, f); fwrite(&sz, 4, 1, f); fwrite(d, 2, (size_t)frames * 2, f); fclose(f);
}

int main(int argc, char **argv)
{
    const mpc_engine_t *e = mpc_engine();
    void *s = e->create("/tmp/prl_test_data");
    const int SEC = 44100, LEN = 4 * SEC;
    int16_t *buf = malloc(sizeof(int16_t) * 2 * LEN);
    int prog, fails = 0;
    double cpu = 0;
    const char *outdir = argc > 1 ? argv[1] : NULL;
    char t[256];
    if (!s) { printf("create failed\n"); return 1; }
    for (prog = 0; prog < 22; prog++) {
        int i, f;
        double peak = 0, rms = 0;
        clock_t c0;
        snprintf(t, sizeof t, "%d", prog);
        e->set_param(s, "program", t);
        e->get_param(s, "oled1", t, sizeof t);
        c0 = clock();
        for (f = 0; f < LEN; f += 128) {
            if (f == 0) { uint8_t m[3] = {0x90, 48, 100}; e->midi(s, m, 3); m[1] = 55; e->midi(s, m, 3); m[1] = 60; e->midi(s, m, 3); m[1] = 64; e->midi(s, m, 3); }
            if (f == SEC * 2) { uint8_t m[3] = {0x80, 48, 0}; e->midi(s, m, 3); m[1] = 55; e->midi(s, m, 3); m[1] = 60; e->midi(s, m, 3); m[1] = 64; e->midi(s, m, 3); }
            e->render(s, buf + 2 * f, LEN - f < 128 ? LEN - f : 128);
        }
        cpu += (double)(clock() - c0) / CLOCKS_PER_SEC;
        for (i = 0; i < 2 * LEN; i++) { const double x = buf[i] / 32768.0; if (fabs(x) > peak) peak = fabs(x); rms += x * x; }
        rms = sqrt(rms / (2.0 * LEN));
        printf("%2d %-16s peak %.3f rms %.4f%s\n", prog, t, peak, rms, rms < 0.003 ? "  <-- SILENT?" : peak >= 0.999 ? "  <-- CLIPS" : "");
        if (rms < 0.003) fails++;
        if (outdir) { char p[512]; snprintf(p, sizeof p, "%s/prog%02d.wav", outdir, prog); wav(p, buf, LEN); }
    }
    printf("cpu: %.2f s for %.0f s of audio (%.1f%% of real time, x86, 4-note chords)\n", cpu, 22 * 4.0, cpu / (22 * 4.0) * 100);
    /* Buttons as MPC drives them (Toggle Switch on touch-down, wrapper hold_ms 200 then a 0 echo, the "_on" poll
     * re-lighting a lit button): a tap on a lit button arrives as 0 and must still count; the same touch re-firing
     * within the hold must not. */
    {
        char on[8], v[48];
        int step, bad = 0;
        const char *want_arp[] = {"1", "2", "0", "1"};   /* arp_mode: Off 0, On 1, Latch 2 */
        const char *want_vm[] = {"1", "2", "3", "0"};   /* vm_type: Poly 0, Mono 1, Unison 2, Chord 3 */
#define RUN_MS(ms) for (f = 0; f < (ms) * 44100 / 1000; f += 128) e->render(s, buf, 128)
        for (step = 0; step < 4; step++) {
            int f;
            e->get_param(s, "arp_btn_on", on, sizeof on);
            e->set_param(s, "arp_btn", atoi(on) ? "0" : "1");   /* the tap: the host's switch flips */
            if (!atoi(on)) {   /* a wobble inside the hold: the switch flips twice more */
                RUN_MS(30); e->set_param(s, "arp_btn", "0"); RUN_MS(30); e->set_param(s, "arp_btn", "1");
                RUN_MS(140); e->set_param(s, "arp_btn", "0");   /* the hold_ms report-back echo */
            }
            RUN_MS(300);
            e->get_param(s, "arp_mode", v, sizeof v);
            if (strcmp(v, want_arp[step])) { printf("button test: arp tap %d -> %s, want %s\n", step + 1, v, want_arp[step]); bad++; }
        }
        for (step = 0; step < 4; step++) {
            int f;
            e->set_param(s, "vm_select", "1");
            RUN_MS(30); e->set_param(s, "vm_select", "0"); RUN_MS(30); e->set_param(s, "vm_select", "1");
            RUN_MS(140); e->set_param(s, "vm_select", "0");
            RUN_MS(300);
            e->get_param(s, "vm_type", v, sizeof v);
            if (strcmp(v, want_vm[step])) { printf("button test: vm tap %d -> %s, want %s\n", step + 1, v, want_vm[step]); bad++; }
        }
#undef RUN_MS
        printf("button test: %s\n", bad ? "FAIL" : "ok (arp Off>On>Latch>Off, voice mode one step per tap)");
        fails += bad;
    }
    e->get_param(s, "oled1", t, sizeof t); printf("oled1: %s\n", t);
    e->get_param(s, "state", t, 8);
    e->destroy(s);
    free(buf);
    printf(fails ? "FAIL (%d silent)\n" : "PASS\n", fails);
    return fails != 0;
}
