/* DrLogueXXL -- 48 kHz -> 44.1 kHz. See resample.h. */
#include "resample.h"
#include <math.h>
#include <string.h>
#include <pthread.h>

static float g_tab[RS_PHASES + 1][RS_TAPS];
static pthread_once_t g_once = PTHREAD_ONCE_INIT;

static double bessel_i0(double x)
{
    double s = 1, t = 1;
    int k;
    for (k = 1; k < 40; k++) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; if (t < 1e-12 * s) break; }
    return s;
}

static void build(void)
{
    const double fc = 20000.0 / RS_IN_RATE, beta = 8.0, i0b = bessel_i0(beta);
    int p, k;
    for (p = 0; p <= RS_PHASES; p++) {
        const double frac = (double)p / RS_PHASES;
        double sum = 0;
        for (k = 0; k < RS_TAPS; k++) {
            const double t = (double)k - (RS_TAPS / 2 - 1) - frac;     /* distance from the output instant */
            const double x = t / (RS_TAPS / 2.0);
            const double w = fabs(x) >= 1 ? 0 : bessel_i0(beta * sqrt(1 - x * x)) / i0b;
            const double s = t == 0 ? 2 * fc : sin(2 * M_PI * fc * t) / (M_PI * t);
            g_tab[p][k] = (float)(s * w);
            sum += s * w;
        }
        for (k = 0; k < RS_TAPS; k++) g_tab[p][k] = (float)(g_tab[p][k] / sum);   /* unity DC gain per phase */
    }
}

void rs_init(resampler_t *r)
{
    pthread_once(&g_once, build);
    memset(r, 0, sizeof *r);
    r->have = RS_TAPS / 2;   /* frames still needed before the first output (zeros stand for the past) */
}

void rs_push(resampler_t *r, float l, float rr)
{
    r->hist[0][r->w] = r->hist[0][r->w + RS_TAPS] = l;
    r->hist[1][r->w] = r->hist[1][r->w + RS_TAPS] = rr;
    r->w = (r->w + 1) % RS_TAPS;
    if (r->have > 0) r->have--;
}

int rs_need(const resampler_t *r) { return r->have; }

void rs_pull(resampler_t *r, float *l, float *rr)
{
    /* window = the RS_TAPS newest frames, oldest first: hist[w .. w+TAPS-1]; the output instant lies `pos` frames
     * after the window's centre tap (RS_TAPS/2 - 1) */
    const double fp = r->pos * RS_PHASES;
    const int p = (int)fp;
    const float a = (float)(fp - p);
    const float *c0 = g_tab[p], *c1 = g_tab[p + 1];
    const float *xl = &r->hist[0][r->w], *xr = &r->hist[1][r->w];
    float sl = 0, sr = 0;
    int k;
    for (k = 0; k < RS_TAPS; k++) {
        const float c = c0[k] + a * (c1[k] - c0[k]);
        sl += c * xl[k];
        sr += c * xr[k];
    }
    *l = sl; *rr = sr;
    r->pos += RS_IN_RATE / RS_OUT_RATE;   /* the next output instant, 48000/44100 input frames later */
    while (r->pos >= 1.0) { r->pos -= 1.0; r->have++; }
}
