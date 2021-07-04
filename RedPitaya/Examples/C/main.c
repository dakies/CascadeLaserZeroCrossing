#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "rp.h"

#define TS 0.000004

// delete fprint
// block vs smaple by sample
// double/float
// scale freq in terms of ts
// variable times variable vs cos cos
// 2*ts constant
// define ts, so its not passed as parameter (or static variable)
// Thermal throttilng
// DVFS

void predict_state(float *state, const float ts) {
    state[0] = state[0] + state[1] * ts;
}

void predict_P(float *P, const float *Q, const float ts) {
    P[0] = P[0] + 2 * ts * P[3] + ts * ts * P[1] + Q[0];
    P[1] = P[1] + Q[1];
    P[2] = P[2] + Q[2];
    P[3] = P[3] + ts * P[1] + Q[3];
}

float alpha(float const *state, float const *P) {
    return P[0] * state[0] * state[0] * cosf(state[0])*cosf(state[0]) + P[2] * sinf(state[0])*sinf(state[0]);
}

float calc_S(const float *state, const float *P, const float R) {

    return alpha(state, P) + R;
}

void gain(float *K, const float *state, const float *P, float r) {
    float s = calc_S(state, P, r);
    K[0] = P[0] * state[2] * cosf(state[0]);
    K[0] = s;
    K[1] = P[3] * state[2] * cosf(state[0]);
    K[1] /= s;
    K[2] = P[2] * sinf(state[0]);
    K[2] /= s;
}

void update_state(float *state, float *P, float measurement,  float r) {
    float y = measurement - state[2] * sinf(state[0]);
    float k[3];
    gain(k, state, P, r);
    state[0] = state[0] + y * k[0];
    state[1] = state[1] + y * k[1];
    state[2] = state[2] + y * k[2];
}

void update_p(float *P, float *state, float r) {
    float a = alpha(state, P);
    a = (1 - a / (a + r));
    P[0] *= a;
    P[1] *= a;
    P[2] *= a;
    P[3] *= a;
}

int main() {
    _Bool half_phase;
    //float phase_prev;
    float x[3];
    float p[4];
    float q[3] = {1,1,1};
    //float k[3];
    float z;
    float r = 0.0001;
    //float s;
    //float a;

    /* Print error, if rp_Init() function failed */
    if (rp_Init() != RP_OK) {
        fprintf(stderr, "Rp api init failed!\n");
    }
    // FILE *fp;
    // fp = fopen("/time_log.txt", "w");


    uint32_t buff_size = 1;
    float *buff1 = (float *) malloc(buff_size * sizeof(float));
    float *buff2 = (float *) malloc(buff_size * sizeof(float));

    rp_AcqReset();
    rp_AcqSetDecimation(RP_DEC_8);
    rp_AcqSetTriggerDelay(0);

    rp_AcqStart();
    //while (1)
    for(int i=0; i<10000 ;++i){
        // Time for iteration of filter
        clock_t begin = clock();

        // Get data from buffer
        rp_AcqGetOldestDataV(RP_CH_1, &buff_size, buff1);
        //rp_AcqGetOldestDataV(RP_CH_2, &buff_size, buff2);

        // If buffer empty, skip...
        z=buff1[0];

        predict_state(x, TS);
        predict_P(p, q, TS);
        update_state(x, p, z, r);
        update_p(p, x, r);

        if(x[0]>2*M_PI){
            x[0] = x[0] - 2*M_PI;
        }
        if(x[0]>M_PI){
            half_phase = 1;
        } else{
            half_phase = 0;
        }


        // End timing
        clock_t end = clock();
        float time_spent = (float) (end - begin); // / CLOCKS_PER_SEC;

        printf("%d", half_phase);

        //Save time
        fprintf(stderr, "%lf\n", time_spent);
    }
    /* Releasinfg resources */
    free(buff1);
    free(buff2);
    rp_Release();

    // fclose(fp);
}
