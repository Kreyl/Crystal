/*
 * adc_f0.cpp
 *
 *  Created on: 16.02.2013
 *      Author: kreyl
 */

#include "adc_f100.h"
#include "hal.h"
#include "kl_lib.h"
#include "board.h"
#include "MsgQ.h"

#include "shell.h"

Adc_t Adc;
const uint8_t AdcChannels[ADC_CHANNEL_CNT] = ADC_CHANNELS;

// Wrapper for IRQ
extern "C" {
void AdcTxIrq(void *p, uint32_t flags) {
    dmaStreamDisable(Adc.PDma);
    Adc.Disable();
    chSysLockFromISR();
    EvtQMain.SendNowOrExitI(EvtMsg_t(evtIdAdcRslt));
    chSysUnlockFromISR();
}
} // extern C

void Adc_t::Init() {
    Clk.SetupAdcClk(adcDiv2);
    rccEnableADC1(FALSE);
    // Enable: may configure only enabled ADC
    ADC1->CR2 |= ADC_CR2_ADON;
    // Enable Vrefint channel, set SwStart as start trigger
    ADC1->CR2 |= ADC_CR2_TSVREFE | (0b111UL << 17) | ADC_CR2_EXTTRIG | ADC_CR2_DMA;
    ADC1->CR1 |= ADC_CR1_SCAN;               // Mode = scan
    // Setup channels
    SetSequenceLength(ADC_SEQ_LEN);
    uint8_t SeqIndx = 1;    // First sequence item is 1, not 0
    for(uint8_t i=0; i < ADC_CHANNEL_CNT; i++) {
        SetChannelSampleTime(AdcChannels[i], ADC_SAMPLE_TIME);
        for(uint8_t j=0; j<ADC_SAMPLE_CNT; j++) SetSequenceItem(SeqIndx++, AdcChannels[i]);
    }
    Calibrate();    // Must be performed once after power-on
    // ==== DMA ====
    PDma = dmaStreamAlloc(ADC_DMA, IRQ_PRIO_LOW, AdcTxIrq, nullptr);
    dmaStreamSetPeripheral(PDma, &ADC1->DR);
    dmaStreamSetMode      (PDma, ADC_DMA_MODE);
    Disable();
}

void Adc_t::StartMeasurement() {
//    Printf("CR1: %X; CR2: %X\r", ADC1->CR1, ADC1->CR2);
    // DMA
    dmaStreamSetMemory0(PDma, IBuf);
    dmaStreamSetTransactionSize(PDma, ADC_SEQ_LEN);
    dmaStreamSetMode(PDma, ADC_DMA_MODE);
    dmaStreamEnable(PDma);
    // ADC
    Enable();
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

uint32_t Adc_t::GetResult(uint8_t AChannel) {
//    for(uint8_t i=0; i<ADC_SEQ_LEN; i++) Printf("%u ", IBuf[i]);
//    PrintfEOL();
    uint32_t Start = AChannel * ADC_SAMPLE_CNT;
    uint32_t Stop  = Start + ADC_SAMPLE_CNT;
    // Average values
    uint32_t Rslt = 0;
    for(uint32_t i = Start; i < Stop; i++) Rslt += IBuf[i];
    return Rslt / ADC_SAMPLE_CNT;
}

void Adc_t::SetSequenceLength(uint32_t ALen) {
    ADC1->SQR1 &= ~ADC_SQR1_L;  // Clear count
    ADC1->SQR1 |= (ALen-1) << 20;
}

void Adc_t::SetChannelSampleTime(uint32_t AChnl, uint32_t ASampleTime) {
    if(AChnl <= 9) {
        uint32_t Offset = AChnl * 3;
        ADC1->SMPR2 &= ~(0b111UL << Offset);
        ADC1->SMPR2 |= ASampleTime << Offset;
    }
    else {
        uint32_t Offset = (AChnl - 10) * 3;
        ADC1->SMPR1 &= ~(0b111UL << Offset);
        ADC1->SMPR1 |= ASampleTime << Offset;
    }
}

void Adc_t::SetSequenceItem(uint8_t SeqIndx, uint32_t AChnl) {
    if(SeqIndx <= 6) {
        uint32_t Offset = (SeqIndx - 1) * 5;
        ADC1->SQR3 &= ~(0b11111UL << Offset);
        ADC1->SQR3 |=  (AChnl << Offset);
    }
    else if(SeqIndx <= 12) {
        uint32_t Offset = (SeqIndx - 7) * 5;
        ADC1->SQR2 &= ~(0b11111UL << Offset);
        ADC1->SQR2 |=  (AChnl << Offset);
    }
    else if(SeqIndx <= 16) {
        uint32_t Offset = (SeqIndx - 13) * 5;
        ADC1->SQR1 &= ~(0b11111UL << Offset);
        ADC1->SQR1 |=  (AChnl << Offset);
    }
}

void Adc_t::Calibrate() {
    // ! Beware: error in datasheet.
    // Before starting a calibration the ADC must have been in __power-on__ state (not power-off) for at least two ADC clock cycles
    Enable();
    for(volatile uint32_t i=0; i<8; i++);
    ADC1->CR2 |= ADC_CR2_RSTCAL;        // Reset calibration
    while(ADC1->CR2 & ADC_CR2_RSTCAL);  // Wait until reset completed
    ADC1->CR2 |= ADC_CR2_CAL;           // Start calibration
    while(ADC1->CR2 & ADC_CR2_CAL);     // Wait until calibration completed
}
