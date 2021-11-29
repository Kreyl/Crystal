#if 1 // ============================ Includes =================================
#include "hal.h"
#include "MsgQ.h"
#include "uart.h"
#include "shell.h"
#include "CrystalLed.h"
#include "ChunkTypes.h"
#include "buttons.h"
#include "SaveToFlash.h"
#include "adc_f100.h"
#include "battery_consts.h"
#endif
#if 1 // ======================== Variables & prototypes =======================
// Forever
bool OsIsInitialized = false;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
void OnCmd(Shell_t *PShell);
void ITask();
//IWDG_t Iwdg;

bool Btn1IsPressed() { return PinIsHi(BTN1_PIN); }
static void EnterSleepNow();
static void EnterSleep();
bool IsEnteringSleep = false;
#endif

#if 1 // === ADC ===
extern Adc_t Adc;
void OnMeasurementDone();
TmrKL_t TmrOneS {TIME_MS2I(999), evtIdEverySecond, tktPeriodic};
#endif

#if 1 // ======================== LEDs related =================================
LedRGBChunk_t lsqOn[] = {
        {csSetup, 600, clGreen},
        {csEnd}
};

LedRGBChunk_t lsqOff[] = {
        {csSetup, 360, clBlack},
        {csEnd},
};

ColorHSV_t hsv(120, 100, 100);
TmrKL_t TmrSave {TIME_MS2I(3600), evtIdTimeToSave, tktOneShot};
#endif

int main(void) {
#if 1 // ==== Get source of wakeup ====
    rccEnablePWRInterface(FALSE);
    if(PWR->CSR & PWR_CSR_WUF) { // Wakeup occured
        // Is it button?
        PinSetupInput(BTN1_PIN, pudPullDown);
        if(Btn1IsPressed()) {
            // Check if pressed long enough
            for(uint32_t i=0; i<540000; i++) {
                // Go sleep if btn released too fast
                if(!Btn1IsPressed()) EnterSleepNow();
            }
            // Btn was keeped in pressed state long enough, proceed with powerOn
        }
    }
#endif

#if 1 // ==== Iwdg, Clk, Os, EvtQ, Uart ====
    // Start Watchdog. Will reset in main thread by periodic 1 sec events.
//    Iwdg::InitAndStart(4500);
//    Iwdg::DisableInDebug();
    Clk.UpdateFreqValues();
    // Init OS
    halInit();
    chSysInit();
    OsIsInitialized = true;
    EvtQMain.Init();
#endif

    // ==== Init hardware ====
    Uart.Init();
    // Remap pins: disable JTAG leaving SWD, T3C2 at PB5, T2C3&4 at PB10&11, USART1 at PB6/7
    AFIO->MAPR = (0b010UL << 24) | (0b10UL << 10) | (0b10UL << 8) | AFIO_MAPR_USART1_REMAP;
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();
//    // Check if IWDG frozen in standby
//    if(!Flash::IwdgIsFrozenInStandby()) {
//        Printf("IWDG not frozen in Standby, frozing\r");
//        chThdSleepMilliseconds(45);
//        Flash::IwdgFrozeInStandby();
//    }

    // Load and check color
    Flash::Load((void*)&hsv, sizeof(ColorHSV_t));
    Printf("Saved clr: %u\r", hsv.H);
    if(hsv.H > 360) hsv.H = 135;
    hsv.S = 100;
    hsv.V = 100;

    // LEDs
    Leds::Init();
    // Set what loaded
    lsqOn[0].Color = hsv.ToRGB();
    Leds::StartSeq(lsqOn);

    // Wait until main button released
    while(Btn1IsPressed()) { chThdSleepMilliseconds(63); }
    SimpleSensors::Init(); // Buttons

    // Battery measurement
    PinSetupAnalog(ADC_BAT_PIN);
    PinSetupOut(ADC_BAT_EN, omPushPull);
    PinSetHi(ADC_BAT_EN);
    Adc.Init();

    TmrOneS.StartOrRestart();

    // Main cycle
    ITask();
}

uint32_t rslt = 0;

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;

            case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                // Main button == BTN1
                if(Msg.BtnEvtInfo.BtnID == 0) {
                    if(Msg.BtnEvtInfo.Type == beLongPress) {
                        IsEnteringSleep = !IsEnteringSleep;
                        if(IsEnteringSleep) {
                            Leds::StartSeq(lsqOff);
                        }
                        else Leds::StartSeq(lsqOn);
                    }
                    else if(Msg.BtnEvtInfo.Type == beRelease) {
                        if(!IsEnteringSleep) Adc.StartMeasurement();
                    }
                }
                // Right / Left buttons == BTN2 & 3
                if((Msg.BtnEvtInfo.BtnID == 1 or Msg.BtnEvtInfo.BtnID == 2) and Msg.BtnEvtInfo.Type != beLongPress) {
                    if(Msg.BtnEvtInfo.BtnID == 1) {
                        if(hsv.H < 360) hsv.H++;
                        else hsv.H = 0;
                    }
                    else if(Msg.BtnEvtInfo.BtnID == 2) {
                        if(hsv.H > 0) hsv.H--;
                        else hsv.H = 360;
                    }
                    lsqOn[0].Color = hsv.ToRGB();
                    Leds::SetAllHsv(hsv);
                    TmrSave.StartOrRestart();
                }
                break;

            case evtIdTimeToSave:
                Flash::Save((uint32_t*)&hsv, sizeof(ColorHSV_t));
                Leds::Off();
                chThdSleepMilliseconds(153);
                Leds::SetAllHsv(hsv);
                break;


            case evtIdEverySecond:
//                Printf("Second\r");
//                Iwdg::Reload();
                if(IsEnteringSleep and Leds::AreOff() and !Btn1IsPressed()) EnterSleep();
                break;

            case evtIdAdcRslt: OnMeasurementDone(); break;

            default: break;
        } // switch
    } // while true
}

void OnMeasurementDone() {
//    Printf("%u %u %u\r", Adc.GetResult(0), Adc.GetResult(1), Adc.Adc2mV(Adc.GetResult(0), Adc.GetResult(1)));
    // Calculate voltage
    uint32_t VBat = 2 * Adc.Adc2mV(Adc.GetResult(0), Adc.GetResult(1)); // *2 because of resistor divider
    uint8_t Percent = mV2PercentAlkaline(VBat);
    Printf("VBat: %umV; Percent: %u\r", VBat, Percent);
    ColorHSV_t tmp = hsv;
    Leds::Off();
    chThdSleepMilliseconds(108);
    if     (Percent <= 20) hsv = {0,   100, 100};
    else if(Percent <  80) hsv = {60,  100, 100};
    else                   hsv = {120, 100, 100};
    Leds::SetAllHsv(hsv);
    chThdSleepMilliseconds(1530);
    Leds::Off();
    hsv = tmp;
    Leds::StartSeq(lsqOn);
}

void EnterSleep() {
    Printf("Entering sleep\r");
    chThdSleepMilliseconds(45);
    chSysLock();
    EnterSleepNow();
    chSysUnlock();
}

void EnterSleepNow() {
    Sleep::EnableWakeupPin(); // Btn0
    Sleep::EnterStandby();
}

#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ok();
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(PCmd->NameIs("Clr")) {
//        uint8_t N;
//        Color_t Clr;
//        if(PCmd->GetNext<uint8_t>(&N) == retvOk) {
//            if(PCmd->GetClrRGB(&Clr) == retvOk) {
//                if(N > 3) for(auto &Led : Leds) Led.SetColor(Clr);
//                else Leds[N].SetColor(Clr);
//            }
//            else PShell->BadParam();
//        }
//        else PShell->BadParam();
    }


    else PShell->CmdUnknown();
}
#endif
