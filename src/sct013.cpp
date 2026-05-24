#include "sct013.h"


void startSamplingCurrent(sct013_val_s *val){
/*
pseudocode : 
- masukin (adcRaw - adcOffset)^2 ke array index *dapetin centered value lalu di squared
- jika udh 200. valuenya dijumlahin semua lalu dibagi 200 *summing window value then mean n root it 
- hasilnya diubah ke voltage lalu convert ke current (1A/5V)
*/

/*
pseudocode : 
- bikin millis sampling 1 ms untuk masukin ke sumsquared
- bikin millis 200ms untuk hitung hasilnya 
*/

    uint32_t now = millis();
    int centeredAdc = 0;

    if(now - val->lastSamplingTime >= SAMPLING_INTERVAL_MS){
        val->lastSamplingTime = now;
        uint16_t adcRaw = analogRead(val->adc_pin);
        centeredAdc = (adcRaw - SCT013_ADC_OFFSET);
        val->sumSquared += centeredAdc * centeredAdc;
        val->cnt++;
    }

    if(now - val->lastUpdateTime >= SAMPLING_WINDOW_MS){
        val->lastUpdateTime = now;

        if(val->cnt > 0){
            val->adcRms = sqrt(val->sumSquared / val->cnt);
            val->voltAdcRms = (val->adcRms * ADC_VOLTAGE_RANGE) / MAX_VAL_ADC;
            val->currRms = val->voltAdcRms * SCT013_FACTOR;
            val->powerRms = 220.0f * val->currRms * 0.9f;
        }
        
        val->sumSquared = 0;
        val->cnt = 0;
    }

}