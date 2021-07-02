#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "rp.h"


void predict_state(double *state, const double ts) {
    state[0] = state[0] + state[1] * ts;
}

void predict_P(double *P, const double *Q, const double ts) {
    P[0] = P[0] + 2 * ts * P[3] + ts * ts * P[1] + Q[0];
    P[1] = P[1] + Q[1];
    P[2] = P[2] + Q[2];
    P[3] = P[3] + ts * P[1] + Q[3];
}

double alpha(double const *state, double const *P) {
    return P[0] * state[0] * state[0] * cos(state[0])*cos(state[0]) + P[2] * sin(state[0])*sin(state[0]);
}

double calc_S(const double *state, const double *P, const double R) {

    return alpha(state, P) + R;
}

void gain(double *K, const double *state, const double *P, double r) {
    double s = calc_S(state, P, r);
    K[0] = P[0] * state[2] * cos(state[0]);
    K[0] = s;
    K[1] = P[3] * state[2] * cos(state[0]);
    K[1] /= s;
    K[2] = P[2] * sin(state[0]);
    K[2] /= s;
}

void update_state(double *state, double *K, double *P, double measurement,  double r) {
    double y = measurement - state[2] * sin(state[0]);
    double k[3];
    gain(k, state, P, r);
    state[0] = state[0] + y * k[0];
    state[1] = state[1] + y * k[1];
    state[2] = state[2] + y * k[2];
}

void update_p(double *P, double *state, double r) {
    double a = alpha(state, P);
    a = (1 - a / (a + r));
    P[0] *= a;
    P[1] *= a;
    P[2] *= a;
    P[3] *= a;
}

int main() {
    const double ts = 0.001;
    _Bool half_phase;
    //double phase_prev;
    double x[3];
    double p[4];
    double q[3] = {1,1,1};
    double k[3];
    double z;
    double r = 0.0001;
    //double s;
    //double a;

    /* Print error, if rp_Init() function failed */
    if (rp_Init() != RP_OK) {
        fprintf(stderr, "Rp api init failed!\n");
    }
    // FILE *fp;
    // fp = fopen("/time_log.txt", "w");


    uint32_t buff_size = 16384;
    float *buff1 = (float *) malloc(buff_size * sizeof(float));
    float *buff2 = (float *) malloc(buff_size * sizeof(float));

    rp_AcqReset();
    rp_AcqSetDecimation(RP_DEC_8);
    rp_AcqSetTriggerDelay(0);

    rp_AcqStart();

    /* After acquisition is started some time delay is needed in order to acquire fresh samples in to buffer*/
    /* Here we have used time delay of one second but you can calculate exact value taking in to account buffer*/
    /*length and smaling rate*/
    sleep(1);
    //fscanf
    rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);
    // rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;


    //while (1)
    for(int i=0; i<10000 ;++i){
        // Time for iteration of filter
        clock_t begin = clock();

        // Get data from buffer
        rp_AcqGetOldestDataV(RP_CH_1, &buff_size, buff1);
        //rp_AcqGetOldestDataV(RP_CH_2, &buff_size, buff2);
        z=buff1[0];

        predict_state(x, ts);
        predict_P(p, q, ts);
        update_state(x, k, p, z, r);
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
        double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;

        printf("%d", half_phase);

        //Save time
        fprintf(stderr, "%lf", time_spent);
    }
    /* Releasing resources */
    free(buff1);
    free(buff2);
    rp_Release();

    // fclose(fp);
}
