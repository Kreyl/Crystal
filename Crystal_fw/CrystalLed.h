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
    uint16_t ClrHMin, ClrHMax;
    uint32_t MinBrt, Saturation;
    EffSettings_t(
            uint32_t ADurMinOff, uint32_t ADurMaxOff, uint32_t ADurMinOn, uint32_t ADurMaxOn,
            uint32_t SmMin, uint32_t SmMax,
            uint16_t AClrHMin, uint16_t AClrHMax,
            uint32_t AMinBrt, uint8_t ASaturation
            ) :
                DurMinOff(ADurMinOff), DurMaxOff(ADurMaxOff), DurMinOn(ADurMinOn), DurMaxOn(ADurMaxOn),
                SmoothMin(SmMin), SmoothMax(SmMax),
                ClrHMin(AClrHMin), ClrHMax(AClrHMax),
                MinBrt(AMinBrt), Saturation(ASaturation)
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

#endif //CRYSTALLED_H__
