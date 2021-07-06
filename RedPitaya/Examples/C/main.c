#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "rp.h"

#define TS 0.0000041

// delete fprint
// block vs sample by sample
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
    float cos_thet = cosf(state[0]);
    float sin_thet = sinf(state[0]);
    return P[0] * state[0] * state[0] * cos_thet*cos_thet + P[2] * sin_thet*sin_thet;
}

float calc_S(const float *state, const float *P, const float R) {

    return alpha(state, P) + R;
}

void gain(float *K, const float *state, const float *P, float r) {
    float s = calc_S(state, P, r);
    float cos_thet = cosf(state[0]);
    K[0] = P[0] * state[2] * cos_thet;
    K[0] = s;
    K[1] = P[3] * state[2] * cos_thet;
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
    _Bool half_phase_prev;
    //float phase_prev;
    float x[3]={0, 15800*2*M_PI, 0.1};
    float q[4] = {0.1,0.1,0.1,0.1};
    float p[4] = {100,100,100, 100};
    //float k[3];
    float z;
    float r = 0.000001;
    //float s;
    //float a;

    /* Print error, if rp_Init() function failed */
    if (rp_Init() != RP_OK) {
        fprintf(stderr, "Rp api init failed!\n");
    }

    // Print current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }

    // Open file to save CPU times
    FILE *fp;
    fp = fopen("./time_log.txt", "w");
    // Check if it managed to open the file
    if (!fp){
        perror("fopen");
        return 0;
    }

    // Open file to save CPU times
    FILE *dat;
    dat = fopen("./resampled.txt", "w");
    // Check if it managed to open the file
    if (!dat){
        perror("fopen");
        return 0;
    }

    uint32_t buff_size = 1;
    float *buff1 = (float *) malloc(buff_size * sizeof(float));
    float *buff2 = (float *) malloc(buff_size * sizeof(float));

    /*
    RP_SMP_244_140K = 512,     //!< Sample rate 244ksps; Buffer time length 67.07ms; Decimation 512
    rp_AcqSetSamplingRate(RP_SMP_244_140K);*/

    rp_AcqReset();
    rp_AcqSetDecimation(RP_DEC_8);
    rp_AcqSetTriggerDelay(0);
    rp_AcqStart();

    //Set digital pin direction
    rp_DpinSetDirection(RP_DIO0_P, RP_OUT);

   /* rp_GenWaveform(RP_CH_1, RP_WAVEFORM_ARBITRARY);
    float *signaaal = (float*) malloc(2 * sizeof(float));
    *signaaal = 0; *(signaaal + 1) = 1; 
    rp_GenArbWaveform(RP_CH_1, signaaal, 2);

    rp_GenMode(RP_CH_1, RP_GEN_MODE_BURST);
    rp_GenBurstCount(RP_CH_1, 1);
    rp_GenBurstRepetitions(RP_CH_1, -1);
    rp_GenBurstPeriod(RP_CH_1, 1);
    rp_GenOutEnable(RP_CH_1);*/
     

    while(1){
        // Time for iteration of filter
        clock_t begin = clock();

        // Get data from buffer
        //rp_AcqGetDataV2
        rp_AcqGetOldestDataV(RP_CH_1, &buff_size, buff1);
        //rp_AcqGetOldestDataV(RP_CH_2, &buff_size, buff2);

        // Todo: If buffer empty, skip...
        z=buff1[0];

        predict_state(x, TS);
        predict_P(p, q, TS);
        update_state(x, p, z, r);
        update_p(p, x, r);

        if(x[0]>2*M_PI){
            x[0] = x[0] - 2*M_PI;
        }
        // Check if phase is greater or smaller than Pi
        if(x[0]>M_PI){
            half_phase = 1;
            rp_DpinSetState(RP_DIO0_P, RP_LOW);
            printf("Up \n");
        } else{
            half_phase = 0;
            rp_DpinSetState(RP_DIO0_P, RP_HIGH);
            printf("Down \n");
        }

        // Save channel 2 on change of phase
        if(half_phase^half_phase_prev){
            /*//Save channel 2 on zero crossing of channel 1
            fprintf(dat, "%lf\n", buff2[0]);*/
            //printf("hello\n");
 	    rp_GenTrigger(1);
	    //rp_GenAmp(half_phase);
	    //rp_GenTrigger(0);
        }

        // End timing
        clock_t end = clock();
        float time_spent = (float) (end - begin); // / CLOCKS_PER_SEC;

        //Save time
        //fprintf(fp, "%lf\n", time_spent);
        //fprintf(stderr, "%lf\n", time_spent);
    }
    /* Releasing resources */
    free(buff1);
    free(buff2);
    rp_Release();

    fclose(fp);
    fclose(dat);
}