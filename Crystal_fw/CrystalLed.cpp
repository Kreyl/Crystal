/*
 * CrystalLed.cpp
 *
 *  Created on: 3 мая 2022 г.
 *      Author: layst
 */

#include "CrystalLed.h"
#include "kl_lib.h"
#include "uart.h"
#include "ch.h"

extern const EffSettings_t *PCurrSettings;

class Lipte_t {
private:
    enum {staIdle, staFadeIn, staOn, staFadeOut} IState = staFadeIn;
    bool IsOn = true;
    uint32_t DurationOff, DurationOn;
    uint32_t Smooth;
    int32_t ITmr;
    // Led settings
    ColorHSV_t IClrPrev, IClrTarget{0, 100, 100};
    uint32_t IClrDelay;
    // PWM
    const PinOutputPWM_t  R, G, B;
public:
    ColorHSV_t IClrCurr;

    Lipte_t(
        const PwmSetup_t ARed, const PwmSetup_t AGreen, const PwmSetup_t ABlue) :
            DurationOff(0), DurationOn(0), Smooth(0), ITmr(0), IClrDelay(0),
            R(ARed), G(AGreen), B(ABlue) {}

    void Init() {
        R.Init();
        G.Init();
        B.Init();
        SetColor(clBlack);
    }

    void Generate() {
        PCurrSettings->Generate(&DurationOff, &DurationOn, &Smooth, &IClrTarget.H);
        IClrTarget.S = PCurrSettings->Saturation;
        IClrCurr = IClrTarget;
        IClrCurr.V = PCurrSettings->MinBrt;
        IClrPrev = IClrCurr;
        ITmr = ClrCalcDelay(IClrCurr.V, Smooth);
    }

    void SetColor(Color_t AColor) {
        R.Set(AColor.R);
        G.Set(AColor.G);
        B.Set(AColor.B);
    }

    void On()  { IsOn = true; }
    void Off() { IsOn = false; }
    bool IsIdle() { return (IState == staIdle and !IsOn); }

    void StopAndSetHsv(ColorHSV_t hsv) {
        chSysLock();
        IsOn = false;
        IState = staIdle;
        ITmr = 0;
        SetColor(hsv.ToRGB());
        chSysUnlock();
    }

    void OnTick() {
        chSysLock();
        if(ITmr > 0) ITmr--;
        if(ITmr <= 0) {
            switch(IState) {
                case staIdle:
                    if(IsOn) {
                        IState = staFadeIn;
                        Generate();
                    }
                    break;

                case staFadeIn:
                    // Check if FadeIn done
                    if(IClrCurr == IClrTarget) {
                        IState = staOn;
                        ITmr = DurationOn;
                    }
                    // Not done
                    else {
                        IClrCurr.V++;
                        ITmr = ClrCalcDelay(IClrCurr.V, Smooth);
                    }
                    break;

                case staOn:
                    IState = staFadeOut;
                    ITmr = ClrCalcDelay(IClrCurr.V, Smooth);
                    break;

                case staFadeOut:
                    // Check if FadeOut done
                    if(IClrCurr.V <= PCurrSettings->MinBrt) {
                        IState = staIdle;
                        ITmr = DurationOff;
                    }
                    // Not done
                    else {
                        IClrCurr.V--;
                        ITmr = ClrCalcDelay(IClrCurr.V, Smooth);
                    }
                    break;
            } // switch
        } // if tmr <= 0
        // Set new color if has changed
        if(IClrCurr != IClrPrev) SetColor(IClrCurr.ToRGB());
        IClrPrev = IClrCurr;
        chSysUnlock();
    }
};

Lipte_t Lipti[LED_CNT] = {
        {LED1_R, LED1_G, LED1_B},
        {LED2_R, LED2_G, LED2_B},
        {LED3_R, LED3_G, LED3_B},
        {LED4_R, LED4_G, LED4_B},
};

void EffSettings_t::Generate(
        uint32_t *PDurationOff, uint32_t *PDurationOn,
        uint32_t *PSmooth, uint16_t *PClrH) const {
    *PDurationOff = Random::Generate(DurMinOff, DurMaxOff);
    *PDurationOn  = Random::Generate(DurMinOn, DurMaxOn);
    *PSmooth      = Random::Generate(SmoothMin, SmoothMax);
    *PClrH        = Random::Generate(ClrHMin, ClrHMax);
}

static THD_WORKING_AREA(waEffThread, 128);
__noreturn
static void EffThread(void *arg) {
    chRegSetThreadName("Eff");
    while(true) {
        chThdSleepMilliseconds(1);
        for(auto &Lipte : Lipti) Lipte.OnTick();
    } // while true
}

namespace CrystalLeds {

void Init() {
    for(auto &Lipte : Lipti) {
        Lipte.Init();
        Lipte.Generate();
    }
    chThdCreateStatic(waEffThread, sizeof(waEffThread), HIGHPRIO, (tfunc_t)EffThread, NULL);
}

void On() {
    for(auto &Lipte : Lipti) Lipte.On();
}

void Off() {
    for(auto &Lipte : Lipti) Lipte.Off();
}

bool AreOff() {
    for(auto &Lipte : Lipti) {
        if(!Lipte.IsIdle()) return false;
    }
    return true;
}

void SetAllHsv(ColorHSV_t hsv) {
    for(auto &Lipte : Lipti) Lipte.StopAndSetHsv(hsv);
}

} // namespace
