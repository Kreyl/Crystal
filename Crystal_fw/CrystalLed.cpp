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

static ColorHSV_t curr_clr{0, 100, 100}, target_clr{0, 100, 100};
static const uint32_t ksmooth = 450;
static virtual_timer_t itmr;

class Lipte_t {
private:
    const PinOutputPWM_t  R, G, B;
public:
    Lipte_t(
        const PwmSetup_t ARed, const PwmSetup_t AGreen, const PwmSetup_t ABlue) :
            R(ARed), G(AGreen), B(ABlue) {}

    void Init() {
        R.Init();
        G.Init();
        B.Init();
        SetColor(clBlack);
    }

    void SetColor(Color_t AColor) {
        R.Set(AColor.R);
        G.Set(AColor.G);
        B.Set(AColor.B);
    }

    void SetHsv(ColorHSV_t hsv) {

        SetColor(hsv.ToRGB());
        chSysUnlock();
    }
};

Lipte_t Lipti[LED_CNT] = {
        {LED1_R, LED1_G, LED1_B},
        {LED2_R, LED2_G, LED2_B},
        {LED3_R, LED3_G, LED3_B},
        {LED4_R, LED4_G, LED4_B},
};

static void TmrCallBack(void* p) {
    curr_clr.Adjust(target_clr);
    for(auto &Lipte : Lipti) Lipte.SetHsv(curr_clr);
}

namespace CrystalLeds {

void Init() {
    for(auto &Lipte : Lipti) Lipte.Init();
}

void SetHsvNow(ColorHSV_t hsv) {
    chVTReset(&itmr);
    for(auto &Lipte : Lipti) Lipte.SetHsv(hsv);
    curr_clr = hsv;
}

void SetHsvSmoothly(ColorHSV_t hsv) {
    chVTReset(&itmr);
    target_clr = hsv;
    if(hsv != curr_clr) chVTSet(&itmr, 45, TmrCallBack, nullptr);
}

} // namespace
