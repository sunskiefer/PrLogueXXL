/* PrLogueXXL -- the digital parts of the prologue run at 48 kHz (logue SDK k_samplerate); MPC's engine contract is 44.1 kHz.
 * The whole machine runs at 48 kHz in UH_BLOCK chunks and this converts its stereo output once, at the edge:
 * windowed-sinc (Kaiser, 48 taps, 512 phases, linearly interpolated), cutoff 20 kHz.
 * Latency: about RS_TAPS/2 input frames (24 / 48000 s, 0.5 ms), reported nowhere because it is below one MPC tick. */
#pragma once
#define RS_TAPS 48
#define RS_PHASES 512
#define RS_IN_RATE 48000.0
#define RS_OUT_RATE 44100.0

typedef struct {
    float hist[2][RS_TAPS * 2];   /* doubled ring so a window is contiguous */
    int w;                        /* write index into the ring (0..RS_TAPS-1) */
    double pos;                   /* the next output instant, 0..1 input frames after the window's centre tap */
    int have;                     /* input frames still needed before the next output */
} resampler_t;

void rs_init(resampler_t *r);
/* Push one input frame. */
void rs_push(resampler_t *r, float l, float rr);
/* Input frames still needed before the next output frame can be made (0 = ready). */
int rs_need(const resampler_t *r);
/* Make one output frame (call only when rs_need() == 0). */
void rs_pull(resampler_t *r, float *l, float *rr);
