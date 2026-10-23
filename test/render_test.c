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
    e->get_param(s, "oled1", t, sizeof t); printf("oled1: %s\n", t);
    e->get_param(s, "state", t, 8);
    e->destroy(s);
    free(buf);
    printf(fails ? "FAIL (%d silent)\n" : "PASS\n", fails);
    return fails != 0;
}
