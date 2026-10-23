/* PrLogueXXL CPU check: 16 voices (UNISON, all engines, effects, L.F. COMP) for 20 s of audio */
#include <stdio.h>
#include <time.h>
#include "engine.h"
int main(void)
{
    const mpc_engine_t *e = mpc_engine();
    void *s = e->create(NULL);
    int16_t buf[256];
    int f;
    clock_t c0;
    uint8_t m[3] = {0x90, 48, 100};
    e->set_param(s, "program", "19");   /* Creep: VPM */
    e->set_param(s, "vm_type", "2");
    e->set_param(s, "vco1_level", "1023"); e->set_param(s, "vco2_level", "1023");
    e->set_param(s, "modfx_sw", "1"); e->set_param(s, "dlrv_sw", "2"); e->set_param(s, "lfc_sw", "1");
    e->midi(s, m, 3);
    c0 = clock();
    for (f = 0; f < 44100 * 20; f += 128) e->render(s, buf, 128);
    printf("16 voices unison + fx: %.2f%% of real time (this machine)\n", (double)(clock() - c0) / CLOCKS_PER_SEC / 20.0 * 100);
    e->destroy(s);
    return 0;
}
