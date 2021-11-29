/*
 * Leds.cpp
 *
 *  Created on: 30 нояб. 2021 г.
 *      Author: layst
 */

#include "CrystalLed.h"

LedRGB_t Leds[4] = {
        {LED1_R, LED1_G, LED1_B},
        {LED2_R, LED2_G, LED2_B},
        {LED3_R, LED3_G, LED3_B},
        {LED4_R, LED4_G, LED4_B},
};

namespace Leds {

void Init() {

}

void SetAllHsv(ColorHSV_t hsv) {
    Color_t Clr = hsv.ToRGB();
    for(auto &Led : Leds) Led.SetColor(Clr);
}

void StartSeq(LedRGBChunk_t *lsq) {
    Printf("lsq %X\r", lsq);
    for(auto &Led : Leds) Led.StartOrRestart(lsq);
}

void Off() { for(auto &Led : Leds) Led.Stop(); }

bool AreOff() {
    for(auto &Led : Leds) {
        if(!Led.IsOff()) return false;
    }
    return true;
}

} // namespace


