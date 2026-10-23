/* PrLogueXXL: the firmware's _osc_white() (logue SDK osc_white): white noise in [-1, 1) */
#include <stdint.h>
static __thread uint32_t s_n = 0x2545F491u;
float _osc_white(void)
{
    uint32_t x = s_n;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    s_n = x;
    return (float)(int32_t)x * (1.f / 2147483648.f);
}
