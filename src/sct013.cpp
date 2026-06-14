#include "sct013.h"


void init_sct013(sct013_val_s *device, uint16_t adcPin){
    device->adc_pin = adcPin;
#ifdef ADCESP32
    analogSetPinAttenuation(device->adc_pin, ADC_11db); 
#endif
}

void startSamplingCurrent(sct013_val_s *val){
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
            float tempCurrRms = val->voltAdcRms * SCT013_FACTOR;
            if(tempCurrRms < 0.12f){ //deadband nya 0.12A, karna esp32 adc nya sampah bat euy (sama perlu tambahin auto calibrate SCT013_ADC_OFFSET nanti)
                val->currRms = 0.0f;
                val->powerRms = 0.0f;
            }else{
                val->currRms = val->voltAdcRms * SCT013_FACTOR;
                val->powerRms = 220.0f * val->currRms * 0.9f;
            }
        }
        val->sumSquared = 0;
        val->cnt = 0;
    }

}

void resetCurrentSamplingValue(sct013_val_s *val){
    val->cnt = 0;
    val->sumSquared = 0;
    val->lastSamplingTime = millis();
    val->lastUpdateTime = millis();
}

void printCurrent(sct013_val_s *val){
    Serial.print("voltAdcRms : "); Serial.println(val->voltAdcRms);
    Serial.print("watt : "); Serial.println(val->powerRms);
    Serial.print("curr : "); Serial.println(val->currRms); Serial.println();
}