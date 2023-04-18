/*
 * adc_f100.h
 *
 *  Edited on: 28.11.2021
 *      Author: kreyl
 */

#ifndef ADC_F100_H__
#define ADC_F100_H__

#include "board.h"
#include "hal.h"

/*
  Example:
case evtIdEverySecond: Adc.StartMeasurement(); break;
case evtIdAdcRslt: OnMeasurementDone(); break;

 void OnMeasurementDone() {
//    Printf("AdcDone\r");
    if(Adc.FirstConversion) Adc.FirstConversion = false;
    else {
        uint32_t VRef_adc = Adc.GetResult(ADC_VREFINT_CHNL);
        uint32_t Vadc = Adc.GetResult(BAT_CHNL);
        uint32_t Vmv = Adc.Adc2mV(Vadc, VRef_adc);
//        Printf("VrefAdc=%u; Vadc=%u; Vmv=%u\r", VRef_adc, Vadc, Vmv);
        uint32_t Battery_mV = Vmv * 2; // Resistor divider
//        Printf("Vbat=%u\r", Battery_mV);
        if(Battery_mV < 2800) {
            Printf("Discharged\r");
            EnterSleep();
        }
    }
}
*/

#define ADC_MAX_SEQ_LEN     16  // 1...16; Const, see ref man p.179
#define VREFINT_mV          1200UL // See datasheet p.36
#define ADC_MAX_VALUE       4095    // const: 2^12

#if (ADC_SEQ_LEN > ADC_MAX_SEQ_LEN) || (ADC_SEQ_LEN == 0)
#error "Wrong ADC channel count and sample count"
#endif

class Adc_t {
private:
    uint16_t IBuf[ADC_SEQ_LEN];
    void SetSequenceLength(uint32_t ALen);
    void SetChannelSampleTime(uint32_t AChnl, uint32_t ASampleTime);
    void SetSequenceItem(uint8_t SeqIndx, uint32_t AChnl);
    void Enable()  { ADC1->CR2 |=  ADC_CR2_ADON; }
    void Calibrate();
public:
    void Init();
    void StartMeasurement();

    void ClockOff() { rccDisableADC1(); }
    uint32_t GetResult(uint8_t AChannel);
    uint32_t Adc2mV(uint32_t AdcChValue, uint32_t VrefValue) {
        return (VREFINT_mV * AdcChValue) / VrefValue;
    }

    // Inner use
    const stm32_dma_stream_t *PDma;
    void Disable() { ADC1->CR2 &= ~ADC_CR2_ADON; }
//    uint32_t GetVDAmV(uint32_t VrefADC) { return ((ADC_VREFINT_CAL * 3000UL) / VrefADC); }

//    void EnableVRef()  { ADC->CCR |=  ADC_CCR_VREFEN; }

//    bool ConversionCompleted() { return (ADC1->SR & ADC_SR_EOC); }
};

// ADC sampling_times
#define ADC_SampleTime_1_5Cycles                     ((uint32_t)0x00000000)
#define ADC_SampleTime_7_5Cycles                     ((uint32_t)0x00000001)
#define ADC_SampleTime_13_5Cycles                    ((uint32_t)0x00000002)
#define ADC_SampleTime_28_5Cycles                    ((uint32_t)0x00000003)
#define ADC_SampleTime_41_5Cycles                    ((uint32_t)0x00000004)
#define ADC_SampleTime_55_5Cycles                    ((uint32_t)0x00000005)
#define ADC_SampleTime_71_5Cycles                    ((uint32_t)0x00000006)
#define ADC_SampleTime_239_5Cycles                   ((uint32_t)0x00000007)

#endif //ADC_F100_H__