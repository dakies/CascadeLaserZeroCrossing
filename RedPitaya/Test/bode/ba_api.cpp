/**
 * $Id: $
 *
 * @brief Red Pitaya library Bode analyzer module interface
 *
 * @Author Red Pitaya
 *
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */
#include <iostream>
#include <fstream>
#include <complex.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
//#include <mutex>
#include "ba_api.h"
#include <chrono>
#include "arm_neon.h"

using namespace std::chrono;

typedef double data_t;

#define EXEC_CHECK_MUTEX(x, mutex){ \
 		int retval = (x); \
 		if(retval != RP_OK) { \
            pthread_mutex_unlock((&mutex)); \
 			return retval; \
 		} \
}


static std::vector<float> calib_data;
static pthread_mutex_t mutex;

data_t RMS(const std::vector<float> &data, int size){
    data_t result = 0;
    for(int i = 0; i < size; i++){
        result += data[i] * data[i];
    }
    result =  sqrt(result / size);
    return result;
}


float l_inter(float a, float b, float f)
{
    return a + f * (b - a);
}

data_t InterpolateLagrangePolynomial (data_t x, data_t* x_values, data_t* y_values, int size)
{
	data_t lagrange_pol = 0;
	data_t basics_pol;

	for (int i = 0; i < size; i++)
	{
		basics_pol = 1;
		for (int j = 0; j < size; j++)
		{
			if (j == i) continue;
			basics_pol *= (x - x_values[j])/(x_values[i] - x_values[j]);		
		}
		lagrange_pol += basics_pol*y_values[i];
	}
	return lagrange_pol;
}

data_t calcPoint(data_t *a, data_t *b,int lenghtArray,int index){
	data_t x = 0;
	int j = 0;
	int i = index - lenghtArray > 0 ? index - lenghtArray : 0;
	for (; i < lenghtArray && i <= index; i++) {
		j = index - i;
		x +=a[i]*b[lenghtArray - j - 1];
	}
	return x;
}

data_t getMaxArg(data_t *a, data_t *b,int start,int stop, int step,int lenghtArray){
	int len = lenghtArray / step;
	if (stop == 0)  stop = len;
	data_t packBufA[len];
	data_t packBufB[len];
	data_t corralate[len];
	int    corralateIndex[len];
	
	for(auto k = 0; k < len ; k++){
		packBufA[k] = a[k*step];
		packBufB[k] = b[k*step];
		corralateIndex[k] = -1;
	}

	for(auto k = start; k <stop ; k++){
		corralate[k] = calcPoint(packBufA,packBufB,len,k);
		corralateIndex[k] = k;
	}
	int maxK = 0;
	int maxCor = 0;
	bool init = true;
	for(auto k = 0; k < len ; k++){
		if (corralateIndex[k] != -1)
			if (corralate[k] > maxCor || init){
				init = false;
				maxCor = corralate[k];
				maxK = k;
			}
	}
	return maxK * step;
}

data_t crossCorrelation(data_t *xSignalArray, data_t *ySignalArray, int lenghtArray,int sepm_Per)
{
    data_t argmax = 0;
    data_t *a = xSignalArray;
	data_t *b = ySignalArray;
	int step = log2(lenghtArray);
	int maxK = -1;
	int start = 0;
	int stop  = 0;
	while(step>=1){
		maxK =getMaxArg(a,b,start/step,stop/step,step,lenghtArray);
		start = maxK - step * 10 < 0 ? 0 : maxK - step * 10;
		stop  = maxK + step * 10 > (lenghtArray * 2 + 1) ? (lenghtArray * 2 + 1) : maxK + step * 10;
		step /=2;
	}

	argmax = maxK;

	if (argmax > 0 && argmax < lenghtArray * 2){
		data_t x_axis[] = { 0 , 1 , 2};
		data_t y_axis[3];

		for(int i = argmax-1,j=0; i <= argmax+1 ;i++,j++){
			y_axis[j] = calcPoint(a,b,lenghtArray,i);
		}
		data_t eps = 0.0001;
		data_t start = 0;
		data_t stop = 2;
		data_t y_start = InterpolateLagrangePolynomial(start,x_axis,y_axis,3);
		data_t y_stop  = InterpolateLagrangePolynomial(stop,x_axis,y_axis,3);
		data_t max_inter = 0;
		while(eps  < (stop - start)){
			data_t z = (stop - start)/2.0 + start;
			data_t y_sub  = InterpolateLagrangePolynomial(z,x_axis,y_axis,3);
			if (y_sub > y_start || y_sub > y_stop){
				if (y_start > y_stop){
					stop = z;
					y_stop = y_sub;
				}else{
					start = z;
					y_start = y_sub;
				}
				max_inter = z;
			}
			else{
				max_inter = y_stop > y_start? stop : start;
				break;
			}
		}
		argmax = argmax-1 + max_inter;
	}
    return argmax;
}

data_t phaseCalculator(data_t freq_HZ, data_t samplesPerSecond, int numSamples,int sepm_Per, data_t *xSamples, data_t *ySamples)
{
    data_t timeShift, phaseShift, timeLine;
    auto argmax = crossCorrelation(xSamples, ySamples, numSamples,sepm_Per);

    timeLine = ((numSamples - 1) / samplesPerSecond);
    timeShift = ((timeLine * argmax) / (numSamples - 1)) + (-timeLine);
    phaseShift = ((2 * M_PI) * fmod((freq_HZ * timeShift), 1.0)) - M_PI;
    if (phaseShift <= -M_PI/2) 
		phaseShift += M_PI;
	else if (phaseShift >= M_PI/2) 
		phaseShift -= M_PI;
	return phaseShift;
}

int rp_BaDataAnalysis(const rp_ba_buffer_t &buffer,
					uint32_t size,
					float samplesPerSecond,
					float _freq, 
					int   samples_period,
					float *gain,
					float *phase_out,
					float input_threshold) 
{
	int ret_value = RP_OK;
	data_t buf1[size];
	data_t buf2[size];
	data_t max_ch1 = -100000;
	data_t max_ch2 = -100000;
	data_t min_ch1 = 100000;
	data_t min_ch2 = 100000;

	for (size_t i = 0; i < size; i++){
		buf1[i] = buffer.ch1[i];
		buf2[i] = buffer.ch2[i];
		// Filtring 
		//buf1[i]  = buf1[i] * ( 0.54 - 0.46 * cos(2*M_PI*i / (data_t)(size-1)));
		//buf2[i]  = buf2[i] * ( 0.54 - 0.46 * cos(2*M_PI*i / (data_t)(size-1)));

		// 
		if ((buf1[i]) > max_ch1) {
			max_ch1 = (buf1[i]);
		}
		if ((buf2[i]) > max_ch2) {
			max_ch2 = (buf2[i]);
		}
		if ((buf1[i]) < min_ch1) {
			min_ch1 = (buf1[i]);
		}
		if ((buf2[i]) < min_ch2) {
			min_ch2 = (buf2[i]);
		}
	}

	data_t sig1_rms =  RMS(buffer.ch1,size);
	data_t sig2_rms =  RMS(buffer.ch2,size);
	*gain = sig2_rms / sig1_rms;
	
	if ((max_ch1 - min_ch1) < input_threshold) ret_value = RP_EIPV;
	if ((max_ch2 - min_ch2) < input_threshold) ret_value = RP_EIPV;
	auto phase2 = phaseCalculator(_freq,samplesPerSecond, size ,samples_period,buf1,buf2);

	/* Phase has to be limited between M_PI and -M_PI. */
	if (phase2 <= -M_PI) 
		phase2 += 2*M_PI;
	else if (phase2 >= M_PI) 
		phase2 -= 2*M_PI;

	*phase_out = phase2 *  (180.0 / M_PI) ;
	return ret_value;
}



float rp_BaCalibGain(float _freq, float _ampl)
{
    for (size_t i = 3; i < calib_data.size(); i += 3) // 3 - freq, ampl, phase
    {
        if (calib_data[i] >= _freq)
        {
			float f0 = calib_data[i-3];
			float f1 = calib_data[i];
			float t =  (f1 - f0) != 0 ? (_freq - f0)/(f1 - f0) : 0;
            return _ampl - l_inter(calib_data[i - 3 + 1], calib_data[i + 1], t);
        }
    }

    return _ampl;
}

float rp_BaCalibPhase(float _freq, float _phase)
{
    for (size_t i = 3; i < calib_data.size(); i += 3) // 3 - freq, ampl, phase
    {
        if (calib_data[i] >= _freq)
        {
			float f0 = calib_data[i-3];
			float f1 = calib_data[i];
			float t =  (f1 - f0) != 0 ? (_freq - f0)/(f1 - f0) : 0;
            return _phase - l_inter(calib_data[i - 3 + 2], calib_data[i + 2], t);
        }
    }

    return _phase;
}


int rp_BaResetCalibration()
{
	calib_data.clear();
	remove(BA_CALIB_FILENAME);
    return RP_OK;
}

int rp_BaReadCalibration()
{
	int ignored __attribute__((unused));
    // if current mode != calibration then load calibration params
    if (calib_data.empty() && access(BA_CALIB_FILENAME, R_OK) == F_OK)
    {
        // fprintf(stderr, "Calibration data exist\n");
        FILE* calF = fopen(BA_CALIB_FILENAME, "r");
        fseek(calF, 0, SEEK_END);
        int size = ftell(calF);
        fseek(calF, 0, SEEK_SET);
        calib_data.resize(size/sizeof(float));
        ignored = fread((void*)calib_data.data(), sizeof(calib_data[0]), size/sizeof(calib_data[0]), calF);
        fclose(calF);
    }
    else
    {
        return RP_RCA;
    }
    return RP_OK;
}

int rp_BaWriteCalib(float _current_freq,float _amplitude,float _phase_out)
{
    FILE* calib_file = nullptr;
    calib_file = fopen(BA_CALIB_FILENAME, "a");
    if (calib_file == nullptr){
        return RP_RCA;
    }
    
    float data[] = {_current_freq, _amplitude, _phase_out};
    fwrite(data, sizeof(float), 3, calib_file);
    fclose(calib_file);
    calib_file = nullptr;
    return RP_OK;
}

bool rp_BaGetCalibStatus(){
	return !calib_data.empty();
}


/* Acquire functions. Callback to the API structure */
int rp_BaSafeThreadAcqPrepare()
{
	pthread_mutex_lock(&mutex);
	EXEC_CHECK_MUTEX(rp_AcqReset(), mutex);
	EXEC_CHECK_MUTEX(rp_AcqSetDecimation(RP_DEC_1), mutex);
	EXEC_CHECK_MUTEX(rp_AcqSetTriggerLevel(RP_T_CH_1, 0), mutex);
	EXEC_CHECK_MUTEX(rp_AcqSetTriggerLevel(RP_T_CH_2, 0), mutex);
	EXEC_CHECK_MUTEX(rp_AcqSetTriggerDelay(0), mutex);
	EXEC_CHECK_MUTEX(rp_AcqSetTriggerSrc(RP_TRIG_SRC_DISABLED), mutex);
	pthread_mutex_unlock(&mutex);
	return RP_OK;
}

/* Generate functions  */
int rp_BaSafeThreadGen(rp_channel_t _channel, float _frequency, float _ampl, float _dc_bias)
{
	pthread_mutex_lock(&mutex);
	EXEC_CHECK_MUTEX(rp_GenAmp(_channel, _ampl), mutex); //LCR_AMPLITUDE
	EXEC_CHECK_MUTEX(rp_GenOffset(_channel, _dc_bias), mutex); // 0.25
	EXEC_CHECK_MUTEX(rp_GenWaveform(_channel, RP_WAVEFORM_SINE), mutex);
	EXEC_CHECK_MUTEX(rp_GenFreq(_channel, _frequency), mutex);
	EXEC_CHECK_MUTEX(rp_GenOutEnable(_channel), mutex);
	usleep(10000);
	pthread_mutex_unlock(&mutex);
	return RP_OK;
}


int rp_BaSafeThreadAcqData(rp_ba_buffer_t &_buffer, int _decimation, int _acq_size, float _trigger)
{
	uint32_t pos = 0;
	uint32_t acq_u_size = _acq_size;
	//uint32_t acq_delay = acq_u_size > ADC_BUFFER_SIZE / 2.0 ? acq_u_size - ADC_BUFFER_SIZE / 2.0 : 0; 
	uint64_t sleep_time = static_cast<uint64_t>(_acq_size) * _decimation / (ADC_SAMPLE_RATE / 1e6);
	sleep_time = sleep_time < 1 ? 1 : sleep_time;
	bool fillState = false;
	
	rp_acq_trig_state_t trig_state = RP_TRIG_STATE_TRIGGERED;

	pthread_mutex_lock(&mutex);
	EXEC_CHECK_MUTEX(rp_AcqSetDecimationFactor(_decimation), mutex);
	EXEC_CHECK_MUTEX(rp_AcqSetTriggerDelay(ADC_BUFFER_SIZE / 2.0), mutex);
	//EXEC_CHECK_MUTEX(rp_AcqSetTriggerDelay(-ADC_BUFFER_SIZE / 2.0 + acq_u_size), mutex);
	EXEC_CHECK_MUTEX(rp_AcqStart(), mutex);
	usleep(sleep_time);
	// Trigger, it is needed for the RP_DEC_1 decimation
	EXEC_CHECK_MUTEX(rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW), mutex);
	
	for (;;) {
		EXEC_CHECK_MUTEX(rp_AcqGetTriggerState(&trig_state), mutex);

		if (trig_state == RP_TRIG_STATE_TRIGGERED) {
			break;
		} else {
			usleep(1);
		}
	}

	while(!fillState){
		
		EXEC_CHECK_MUTEX(rp_AcqGetBufferFillState(&fillState), mutex);
	}

	EXEC_CHECK_MUTEX(rp_AcqStop(), mutex); // Why the write pointer is not stopped?
	EXEC_CHECK_MUTEX(rp_AcqGetWritePointerAtTrig(&pos), mutex);
	pos++;
	EXEC_CHECK_MUTEX(rp_AcqGetDataV2(pos, &acq_u_size, _buffer.ch1.data(), _buffer.ch2.data()), mutex);
	pthread_mutex_unlock(&mutex);
	return RP_OK;
}


int rp_BaGetAmplPhase(float _amplitude_in, float _dc_bias, int _periods_number, rp_ba_buffer_t &_buffer, float* _amplitude, float* _phase, float _freq,float _input_threshold)
{
    float gain = 0;
    float phase_out = 0;
    int acq_size;

    //Generate a sinusoidal wave form
    rp_BaSafeThreadGen(RP_CH_1, _freq, _amplitude_in, _dc_bias);
    int size_buff_limit = ADC_BUFFER_SIZE;
	int new_dec = round((static_cast<float>(_periods_number) * ADC_SAMPLE_RATE) / (_freq * size_buff_limit));
	new_dec = new_dec < 1 ? 1 : new_dec;
	
	if (new_dec < 16){
        if (new_dec >= 8)
            new_dec = 8;
        else
            if (new_dec >= 4)
                new_dec = 4;
            else
                if (new_dec >= 2)
                    new_dec = 2;
                else 
                    new_dec = 1;
    }
    if (new_dec > 65536){
        new_dec = 65536;
    }
	int samples_period = round(ADC_SAMPLE_RATE / (_freq * new_dec));

    acq_size = size_buff_limit;
    rp_BaSafeThreadAcqData(_buffer,new_dec, acq_size,_amplitude_in);
    rp_GenOutDisable(RP_CH_1);
    int ret = rp_BaDataAnalysis(_buffer, acq_size, ADC_SAMPLE_RATE / new_dec,_freq, samples_period,  &gain, &phase_out,_input_threshold);

    *_amplitude = 10.*logf(gain);
    *_phase = phase_out;
	if (std::isnan(*_amplitude) || std::isinf(*_amplitude)) ret =  RP_EOOR;
    return ret;
}

