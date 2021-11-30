/*
 * CrystalLed.h
 *
 *  Created on: 30 нояб. 2021 г.
 *      Author: layst
 */

#pragma once

#include "color.h"
#include "ChunkTypes.h"

#define LED_CNT     4

class CrystalLed_t : public BaseSequencer_t<LedRGBChunk_t> {
private:
    const PinOutputPWM_t R[LED_CNT] = { LED1_R, LED2_R, LED3_R, LED4_R };
    const PinOutputPWM_t G[LED_CNT] = { LED1_G, LED2_G, LED3_G, LED4_G };
    const PinOutputPWM_t B[LED_CNT] = { LED1_B, LED2_B, LED3_B, LED4_B };
    Color_t ICurrColor;
    void ISwitchOff() {
        SetColor(clBlack);
        ICurrColor = clBlack;
    }
    SequencerLoopTask_t ISetup() {
        if(ICurrColor != IPCurrentChunk->Color) {
            if(IPCurrentChunk->Value == 0) {     // If smooth time is zero,
                SetColor(IPCurrentChunk->Color); // set color now,
                ICurrColor = IPCurrentChunk->Color;
                IPCurrentChunk++;                // and goto next chunk
            }
            else {
                ICurrColor.Adjust(IPCurrentChunk->Color);
                SetColor(ICurrColor);
                // Check if completed now
                if(ICurrColor == IPCurrentChunk->Color) IPCurrentChunk++;
                else { // Not completed
                    // Calculate time to next adjustment
                    uint32_t Delay = ICurrColor.DelayToNextAdj(IPCurrentChunk->Color, IPCurrentChunk->Value);
                    SetupDelay(Delay);
                    return sltBreak;
                } // Not completed
            } // if time > 256
        } // if color is different
        else IPCurrentChunk++; // Color is the same, goto next chunk
        return sltProceed;
    }

public:
    void Init() {
        for(auto &chnl : R) chnl.Init();
        for(auto &chnl : G) chnl.Init();
        for(auto &chnl : B) chnl.Init();
        SetColor(clBlack);
    }

    bool IsOff() { return (ICurrColor == clBlack) and IsIdle(); }

    void SetColor(Color_t AColor) {
        for(auto &chnl : R) chnl.Set(AColor.R);
        for(auto &chnl : G) chnl.Set(AColor.G);
        for(auto &chnl : B) chnl.Set(AColor.B);
    }

    void SetAllHsv(ColorHSV_t hsv) {
        Color_t rgb = hsv.ToRGB();
        SetColor(rgb);
        ICurrColor = rgb;
    }
};
