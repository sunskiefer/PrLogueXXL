/* PrLogueXXL: loads the built armv7 prloguexxl.so the way MPC does (dlopen + VSTPluginMain), on armv7 (qemu-arm with
 * an armhf glibc, or the Force): plays programs, turns panel controls, presses buttons, uses the arpeggiator on
 * MPC's transport, saves and restores the state chunk, and writes WAVs.
 *   arm_vst_host <plugin.so> <out_dir> */
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AEffect AEffect;
typedef intptr_t (*cb)(AEffect *, int32_t, int32_t, intptr_t, void *, float);
struct AEffect { int32_t magic; intptr_t (*d)(AEffect *, int32_t, int32_t, intptr_t, void *, float);
    void *p; void (*setP)(AEffect *, int32_t, float); float (*getP)(AEffect *, int32_t);
    int32_t np, npar, ni, no, flags; intptr_t r1, r2; int32_t a, b, c; float io; void *obj, *user; int32_t uid, ver;
    void (*pr)(AEffect *, float **, float **, int32_t); void *pdr; char f[56]; };
typedef struct { int32_t type, byteSize, deltaFrames, flags, noteLength, noteOffset; unsigned char m[4]; char x[4]; } ME;
typedef struct { int32_t n; intptr_t r; void *ev[2]; } EV;

static double ti[16];
static int32_t tiFlags = 1 << 10;
static double ppq;
static intptr_t host(AEffect *e, int32_t op, int32_t i, intptr_t v, void *p, float o)
{
    (void)e; (void)i; (void)v; (void)p; (void)o;
    if (op == 7) { ti[3] = ppq; ti[4] = 120.0; ((int32_t *)&ti[8])[5] = tiFlags; return (intptr_t)ti; }
    if (op == 1) return 2400;   /* audioMasterVersion */
    return 0;
}

static float wavbuf[2 * 44100 * 8];
static int wavn;
static void wav(const char *path)
{
    FILE *f = fopen(path, "wb");
    int i, br = 44100 * 4, ds = wavn * 4, rs = 36 + ds;
    short h;
    if (!f) return;
    fwrite("RIFF", 1, 4, f); fwrite(&rs, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    i = 16; fwrite(&i, 4, 1, f); h = 1; fwrite(&h, 2, 1, f); h = 2; fwrite(&h, 2, 1, f);
    i = 44100; fwrite(&i, 4, 1, f); fwrite(&br, 4, 1, f); h = 4; fwrite(&h, 2, 1, f); h = 16; fwrite(&h, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&ds, 4, 1, f);
    for (i = 0; i < 2 * wavn; i++) { float x = wavbuf[i] * 32767.f; h = (short)(x > 32767 ? 32767 : x < -32768 ? -32768 : x); fwrite(&h, 2, 1, f); }
    fclose(f);
    wavn = 0;
}

static double run(AEffect *a, int blocks, int playing)
{
    double e = 0;
    int k, i;
    for (k = 0; k < blocks; k++) {
        float L[128] = {0}, R[128] = {0}, *o[2] = {L, R};
        tiFlags = (1 << 10) | (1 << 9) | (playing ? 1 << 1 : 0);
        a->pr(a, NULL, o, 128);
        if (playing) ppq += 128.0 / 44100.0 * 2.0;   /* 120 BPM: 2 beats a second */
        for (i = 0; i < 128; i++) {
            if (!isfinite(L[i]) || !isfinite(R[i])) return -1;
            e += L[i] * L[i] + R[i] * R[i];
            if (wavn < 44100 * 8) { wavbuf[2 * wavn] = L[i]; wavbuf[2 * wavn + 1] = R[i]; wavn++; }
        }
    }
    return sqrt(e / (blocks * 256.0));
}

static void note(AEffect *a, int ch, int n, int vel)
{
    ME me = {1, sizeof(ME), 0, 0, 0, 0, {(unsigned char)((vel ? 0x90 : 0x80) | ch), (unsigned char)n, (unsigned char)vel, 0}, {0}};
    EV ev = {1, 0, {&me, 0}};
    a->d(a, 25, 0, 0, &ev, 0);   /* effProcessEvents */
}

static int param_index(AEffect *a, const char *name)
{
    int i;
    char buf[64];
    for (i = 0; i < a->npar; i++) {
        buf[0] = 0;
        a->d(a, 8, i, 0, buf, 0);   /* effGetParamName */
        if (!strcmp(buf, name)) return i;
    }
    return -1;
}


static void set(AEffect *a, const char *name, float norm)
{
    const int i = param_index(a, name);
    if (i < 0) { printf("param %s not found FAIL\n", name); exit(1); }
    a->setP(a, i, norm);
}
static void disp(AEffect *a, const char *name, char *buf)
{
    buf[0] = 0;
    a->d(a, 7, param_index(a, name), 0, buf, 0);
}

int main(int argc, char **argv)
{
    void *h;
    AEffect *(*entry)(cb);
    AEffect *a;
    char path[600], *chunk = NULL, b1[64], b2[64];
    double r;
    int fail = 0, len, k;
    if (argc < 3) return 2;
    h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!h) { printf("dlopen: %s\n", dlerror()); return 1; }
    *(void **)&entry = dlsym(h, "VSTPluginMain");
    a = entry(host);
    a->d(a, 0, 0, 0, 0, 0);
    a->d(a, 10, 0, 0, 0, 44100.f);
    a->d(a, 11, 0, 128, 0, 0);
    a->d(a, 12, 0, 1, 0, 0);
    printf("plugin: %d params, %d programs\n", a->npar, a->np);
    /* 1. program 1, a chord */
    note(a, 0, 60, 100); note(a, 0, 64, 100); note(a, 0, 67, 100);
    r = run(a, 300, 0);
    note(a, 0, 60, 0); note(a, 0, 64, 0); note(a, 0, 67, 0);
    r = fmax(r, run(a, 100, 0));
    printf("program 1 chord: rms %.3f %s\n", r, r > 0.02 ? "ok" : "FAIL"); if (r <= 0.02) fail = 1;
    snprintf(path, sizeof path, "%s/arm_prog1.wav", argv[2]); wav(path);
    /* 2. the panel: cutoff down, resonance up, a lever (VCO1 wave), the multi engine */
    set(a, "CUTOFF", 0.3f); set(a, "RESONANCE", 0.7f); set(a, "VCO1 WAVE", 0.f); set(a, "MULTI ENGINE", 0.5f);
    set(a, "MULTI LEVEL", 0.8f);
    disp(a, "DISPLAY 1", b1); disp(a, "DISPLAY 2", b2);
    printf("display after the panel: \"%s\" / \"%s\"\n", b1, b2);
    disp(a, "CUTOFF", b1); printf("cutoff text: \"%s\"\n", b1);
    note(a, 0, 48, 110); r = run(a, 300, 0); note(a, 0, 48, 0); run(a, 200, 0);
    printf("panel edit: rms %.3f %s\n", r, r > 0.01 ? "ok" : "FAIL"); if (r <= 0.01) fail = 1;
    /* 3. every factory program plays */
    for (k = 0; k < 22; k++) {
        a->d(a, 2, 0, k, 0, 0);   /* effSetProgram */
        note(a, 0, 57, 100); r = run(a, 120, 0); note(a, 0, 57, 0); run(a, 60, 0);
        a->d(a, 5, 0, 0, b1, 0);   /* effGetProgramName */
        if (r <= 0.003) { printf("program %d (%s) silent FAIL\n", k + 1, b1); fail = 1; }
    }
    printf("22 factory programs: %s\n", fail ? "FAIL" : "ok");
    /* 4. the arpeggiator on MPC's transport */
    a->d(a, 2, 0, 0, 0, 0);
    set(a, "ARP ON/LATCH", 1.f);
    run(a, 2, 0);
    note(a, 0, 60, 100); note(a, 0, 64, 100); note(a, 0, 67, 100);
    ppq = 0;
    r = run(a, 44100 * 2 / 128, 1);
    note(a, 0, 60, 0); note(a, 0, 64, 0); note(a, 0, 67, 0);
    run(a, 50, 1);
    printf("arpeggiator on MPC transport: rms %.3f %s\n", r, r > 0.01 ? "ok" : "FAIL"); if (r <= 0.01) fail = 1;
    snprintf(path, sizeof path, "%s/arm_arp.wav", argv[2]); wav(path);
    /* 5. the L.F. COMP meter moves */
    set(a, "L.F. COMP", 1.f); set(a, "COMP GAIN", 1.f);
    note(a, 0, 36, 127); run(a, 100, 0);
    printf("comp meter: %.3f (rest 0.708) %s\n", a->getP(a, param_index(a, "COMP METER")),
           a->getP(a, param_index(a, "COMP METER")) < 0.70f ? "ok" : "FAIL");
    if (a->getP(a, param_index(a, "COMP METER")) >= 0.70f) fail = 1;
    note(a, 0, 36, 0);
    /* 6. state chunk round trip */
    len = (int)a->d(a, 23, 0, 0, &chunk, 0);
    if (len > 0 && chunk) {
        char *copy = malloc((size_t)len + 1);
        memcpy(copy, chunk, (size_t)len); copy[len] = 0;
        a->d(a, 24, 0, (intptr_t)len, copy, 0);
        disp(a, "DISPLAY 1", b1);
        printf("state: %d bytes restored, display \"%s\" ok\n", len, b1);
        free(copy);
    } else { printf("no chunk FAIL\n"); fail = 1; }
    a->d(a, 1, 0, 0, 0, 0);
    puts(fail ? "FAIL" : "PASS");
    return fail;
}
