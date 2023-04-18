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
#include "radio_lvl1.h"
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
#define SLEEP_TIMEOUT_S     7
uint8_t TmrSleep;
#endif

#if 1 // ======================== LEDs related =================================
const EffSettings_t EffSettings[7] = {
     //  Off     On       Smooth     Color1 H    Color2 H
        {9, 45,  9, 54,   108, 360,   330, 360,   215, 230}, // 0 Requiem
        {9, 45,  9, 54,   270, 630,   161, 195,    50,  77}, // 1 Grig
        {9, 45,  9, 54,   108, 360,     0,  15,   250, 270}, // 2 tango
        {9, 45,  9, 54,   405, 630,    80, 160,   260, 290}, // 3 waltz
        {9, 45, 99, 99,   108, 810,     0,   7,   353, 360}, // 4 valkirie
        {9, 45, 99, 99,   108, 630,   225, 250,   250, 265}, // 5 dance
        {9, 45,  9, 54,   270, 405,   120, 270,    50,  77}, // 6
};

const EffSettings_t *PCurrSettings = &EffSettings[0];
#endif

int main(void) {
#if 0 // ==== Get source of wakeup ====
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
    Iwdg::InitAndStart(4500);
    Iwdg::DisableInDebug();
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

    // Check if was in standby
    rccEnablePWRInterface(FALSE);
//    if(Sleep::WasInStandby()) { // Was in standby => is sleeping; check radio
//        if(!Sleep::WakeUpOccured()) { // was not woke up by button
//            if(Radio.InitAndRxOnce() == retvOk) {
//                PCurrSettings = &EffSettings[Radio.PktRx.BtnIndx]; // <7 check is inside
//            }
//            else { // Noone here
//                Printf(".");
//                chSysLock();
//                EnterSleepNow();
//                chSysUnlock();
//            }
//        } // if button
//    }

    // Power-on, or radio pkt received => proceed with init
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    // LEDs
    CrystalLeds::Init();
    CrystalLeds::SetAllHsv(ColorHSV_t(180, 100, 100));

    // Wait until main button released
//    while(Btn1IsPressed()) { chThdSleepMilliseconds(63); }
    SimpleSensors::Init(); // Buttons

    // Battery measurement
    PinSetupAnalog(ADC_BAT_PIN);
    PinSetupOut(ADC_BAT_EN, omPushPull);
    PinSetHi(ADC_BAT_EN);
    Adc.Init();

//    Radio.Init();

    TmrOneS.StartOrRestart();
    TmrSleep = SLEEP_TIMEOUT_S;

    // Main cycle
    ITask();

    return 0;
}

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
                    Adc.StartMeasurement();
                }
                break;

            case evtIdRadioCmd:
                IsEnteringSleep = false;
                TmrSleep = SLEEP_TIMEOUT_S;
                CrystalLeds::On();
                if(Msg.BtnIndx < 7) PCurrSettings = &EffSettings[Msg.BtnIndx];
                break;

            case evtIdEverySecond:
//                Printf("Second\r");
                Iwdg::Reload();
//                if(TmrSleep == 0) {
//                    IsEnteringSleep = true;
//                    CrystalLeds::Off();
//                }
//                else TmrSleep--;
//                if(IsEnteringSleep and CrystalLeds::AreOff()) EnterSleep();
                break;

            case evtIdAdcRslt:
                OnMeasurementDone();
                IsEnteringSleep = false;
                TmrSleep = SLEEP_TIMEOUT_S;
                break;

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
    ColorHSV_t hsv;
    if     (Percent <= 20) hsv = {0,   100, 100};
    else if(Percent <  80) hsv = {30,  100, 100};
    else                   hsv = {120, 100, 100};
    CrystalLeds::SetAllHsv(hsv);
    chThdSleepMilliseconds(1530);
    if(Sleep::WakeUpOccured()) EnterSleep();
    else CrystalLeds::On();
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
    Sleep::ClearStandbyFlag();
    Sleep::EnterStandby();
}

#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ok();
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(PCmd->NameIs("N")) {
        uint8_t N;
        if(PCmd->GetNext<uint8_t>(&N) == retvOk) {
            if(N <= 7) {
                PCurrSettings = &EffSettings[N];
                PShell->Ok();
            }
            else PShell->BadParam();
        }
//            if(PCmd->GetClrRGB(&Clr) == retvOk) {
//                if(N > 3) for(auto &Led : Leds) Led.SetColor(Clr);
//                else Leds[N].SetColor(Clr);
//            }
//            else PShell->BadParam();
//        }
        else PShell->BadParam();
    }

    else if(PCmd->NameIs("On")) {
        CrystalLeds::On();
        PShell->Ok();
    }

    else if(PCmd->NameIs("Off")) {
        CrystalLeds::Off();
        PShell->Ok();
    }

    else PShell->CmdUnknown();
}
#endif
