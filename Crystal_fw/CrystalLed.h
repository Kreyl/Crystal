/*
 * CrystalLed.h
 *
 *  Created on: 30 нояб. 2021 г.
 *      Author: layst
 */

#pragma once

#include "ChunkTypes.h"

#define LED_CNT     4

struct EffSettings_t {
    uint32_t DurMinOff, DurMaxOff;
    uint32_t DurMinOn,  DurMaxOn;
    uint32_t SmoothMin, SmoothMax;
    uint16_t Clr1HMin, Clr1HMax;
    uint16_t Clr2HMin, Clr2HMax;
    EffSettings_t(
            uint32_t ADurMinOff, uint32_t ADurMaxOff, uint32_t ADurMinOn, uint32_t ADurMaxOn,
            uint32_t SmMin, uint32_t SmMax,
            uint16_t AClr1HMin, uint16_t AClr1HMax,
            uint16_t AClr2HMin, uint16_t AClr2HMax
            ) :
                DurMinOff(ADurMinOff), DurMaxOff(ADurMaxOff), DurMinOn(ADurMinOn), DurMaxOn(ADurMaxOn),
                SmoothMin(SmMin), SmoothMax(SmMax),
                Clr1HMin(AClr1HMin), Clr1HMax(AClr1HMax),
                Clr2HMin(AClr2HMin), Clr2HMax(AClr2HMax)
                {}
    void Generate(uint32_t *DurationOff, uint32_t *DurationOn, uint32_t *Smooth, uint16_t *ClrH) const;
};


namespace CrystalLeds {

void Init();
void On();
void Off();
bool AreOff();
void SetAllHsv(ColorHSV_t hsv);

} // namespace
