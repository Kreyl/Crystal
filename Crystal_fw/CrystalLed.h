/*
 * CrystalLed.h
 *
 *  Created on: 30 нояб. 2021 г.
 *      Author: layst
 */

#ifndef CRYSTALLED_H__
#define CRYSTALLED_H__

#include "ChunkTypes.h"

#define LED_CNT     4

struct EffSettings_t {
    uint32_t DurMinOff, DurMaxOff;
    uint32_t DurMinOn,  DurMaxOn;
    uint32_t SmoothMin, SmoothMax;
    Color_t Clr;
    uint32_t MinBrt;
    EffSettings_t(
            uint32_t ADurMinOff, uint32_t ADurMaxOff, uint32_t ADurMinOn, uint32_t ADurMaxOn,
            uint32_t SmMin, uint32_t SmMax,
            Color_t AClr, uint32_t AMinBrt
            ) :
                DurMinOff(ADurMinOff), DurMaxOff(ADurMaxOff), DurMinOn(ADurMinOn), DurMaxOn(ADurMaxOn),
                SmoothMin(SmMin), SmoothMax(SmMax),
                Clr(AClr), MinBrt(AMinBrt)
                {}
    void Generate(uint32_t *DurationOff, uint32_t *DurationOn, uint32_t *Smooth) const;
};


namespace CrystalLeds {

void Init();
void On();
void Off();
bool AreOff();
void SetAllHsv(ColorHSV_t hsv);

} // namespace

#endif //CRYSTALLED_H__
