#ifndef __MY_SCT013_H__
#define __MY_SCT013_H__

#include "Arduino.h"
#include "math.h"


#define ADCESP32 //comment ini kalo pake arduino
#ifdef ADCESP32
#define MAX_VAL_ADC         4095.0f //12bit adc, esp32
#define ADC_VOLTAGE_RANGE   3.3f
#else
#define MAX_VAL_ADC     1023 //10bit adc, arduino
#define ADC_VOLTAGE_RANGE   5.0f
#endif

#define SCT013_FACTOR  5 //1V/5A

#define SAMPLING_WINDOW_MS         200
#define MAX_BLOCKING_ALLOWED_MS    50
#define MAX_SAMPLING_IDX           SAMPLING_WINDOW_MS - 1
#define SAMPLING_INTERVAL_MS       1
#define SCT013_ADC_OFFSET          2900



typedef struct{
    float adcRms = 0;
    float voltAdcRms = 0;
    float currRms = 0;
    float powerRms = 0;
    float kwhRms = 0;

    int adc_pin;
    uint16_t cnt = 0;
    float sumSquared = 0.0f;
    uint32_t lastSamplingTime = 0;
    uint32_t lastUpdateTime = 0;
}sct013_val_s;

void startSamplingCurrent(sct013_val_s *val);
void init_sct013(sct013_val_s *device, uint16_t adcPin);
void printCurrent(sct013_val_s *val);
void resetCurrentSamplingValue(sct013_val_s *val);

#endif