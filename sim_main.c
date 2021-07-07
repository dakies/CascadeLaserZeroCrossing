#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>



#define lambda_hene 0.0000006328 //Wavelength of HeNe laser (m)
#define v_stage 0.005 //velocity stage (ms^-1)
#define amplitude 0.012 // Amplitude of HeNe laser signal(V)
#define R 0.000001 //Variance of measurement
#define fs 250000 //Sampling frequency (cross check with rp_AcqSetSamplingRate(***))

#define signal_freq (2*v_stage/lambda_hene) //Frequency of intensity signal
#define omega (2*M_PI*signal_freq) //Angular freq of intesity signal
#define TS (1/fs) // Sampling period

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

float calc_S(const float *state, const float *P) {

    return alpha(state, P) + R;
}

void gain(float *K, const float *state, const float *P) {
    float s = calc_S(state, P);
    float cos_thet = cosf(state[0]);
    K[0] = P[0] * state[2] * cos_thet;
    K[0] = s;
    K[1] = P[3] * state[2] * cos_thet;
    K[1] /= s;
    K[2] = P[2] * sinf(state[0]);
    K[2] /= s;
}

void update_state(float *state, float *P, float measurement) {
    float y = measurement - state[2] * sinf(state[0]);
    float k[3];
    gain(k, state, P);
    state[0] = state[0] + y * k[0];
    state[1] = state[1] + y * k[1];
    state[2] = state[2] + y * k[2];
}

void update_p(float *P, float *state) {
    float a = alpha(state, P);
    a = (1 - a / (a + R));
    P[0] *= a;
    P[1] *= a;
    P[2] *= a;
    P[3] *= a;
}

int main() {
    _Bool half_phase;
    _Bool half_phase_prev;
    //float phase_prev;
    float x[3]={0, omega, amplitude};
    float const q[4] = {0.01,0.001,0.001,0};
    float p[4] = {100,100,100, 100};
    float z;


    // Open file to save CPU times
    FILE *hp;
    hp = fopen("./kalman_c_hp.txt", "w");

    // Check if it managed to open the file
    if (!hp){
        perror("fopen");
        return 0;
    }

    // Open file to save CPU times
    FILE *orig;
    orig = fopen("./kalman_c_orig.txt", "w");

    // Check if it managed to open the file
    if (!orig){
        perror("fopen");
        return 0;
    }
     // Open file to save CPU times
    FILE *filt;
    filt = fopen("./kalman_filt.txt", "w");

    // Check if it managed to open the file
    if (!filt){
        perror("fopen");
        return 0;
    }

    // Open file to save CPU times
    FILE *hene;
    hene = fopen("hene.txt", "r");

    // Check if it managed to open the file
    if (!hene){
        perror("fopen");
        return 0;
    }

    while(fscanf(hene, "%f", &z)== 1){
        predict_state(x, TS);
        predict_P(p, q, TS);
        update_state(x, p, z);
        update_p(p, x);

        if(x[0]>2*M_PI){
            x[0] = x[0] - 2*M_PI;
        }
        // Check if phase is greater or smaller than Pi
        if(x[0]>M_PI){
            half_phase = 1;
            //printf("Up \n");
        } else{
            half_phase = 0;
            //printf("Down \n");
        }

        // Save channel 2 on change of phase
        if(half_phase^half_phase_prev){
            //printf("***ZERO***\n");
        }

        //Save time
        fprintf(hp, "%d\n", half_phase);
        fprintf(orig, "%f\n", z);
        fprintf(filt, "%f\n", x[2]*sinf(x[0]));
    }
    //fclose(fp);
    //fclose(dat);
}
