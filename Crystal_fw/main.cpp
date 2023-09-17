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

//extern Adc_t Adc;
//void OnMeasurementDone();
TmrKL_t TmrOneS {TIME_MS2I(999), evtIdEverySecond, tktPeriodic};
uint32_t RadioTimeout_s = 4;

#if 1 // ======================== LEDs related =================================
#define V_NONE      0
#define V_OFF       1
#define V_ON        2
#define V_MODE_0    0
#define V_MODE_1    1
#define V_MODE_2    2
#define V_MODE_3    3

#define SETTINGS_CNT    4
// Warm white, yellow, red, blue
EffSettings_t EffSettings[SETTINGS_CNT] = {
        //  Off      On        Smooth       Color        MinBrt
        {45, 108,  45, 108,   270, 450, {255, 130,  50},  100},
        {45, 108,  45, 108,   270, 450, {255,   0,   0},   50}, // 1 Red
        {45, 108,  45, 108,   270, 450, {255,  72,   0},  100}, // 2 Yellow
        {45, 108,  45, 108,   270, 450, {  0,   0, 255},   50}, // 3 Blue
};

EffSettings_t *PCurrSettings = &EffSettings[0];
#endif

int main(void) {
#if 1 // ==== Iwdg, Clk, Os, EvtQ, Uart ====
    // Start Watchdog. Will reset in main thread by periodic 1 sec events.
    Iwdg::InitAndStart(4500);
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
    if(Sleep::WasInStandby()) { // Was in standby => is sleeping; check radio
        Printf(".");
        if(Radio.InitAndRxOnce() != retvOk) { // Noone here
            chSysLock();
            EnterSleepNow();
            chSysUnlock();
        }
    }

    // Power-on, or radio pkt received => proceed with init
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    CrystalLeds::Init();
    Radio.Init();
    TmrOneS.StartOrRestart();

    // Main cycle
    ITask();
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

            case evtIdRadioCmd:
                Printf("RCmd %d %d\r", Msg.ValueID, Msg.Value);
                if(Msg.ValueID == V_OFF) {
                    CrystalLeds::Off();
                    RadioTimeout_s = 4;
                }
                else if(Msg.ValueID == V_ON and Msg.Value < SETTINGS_CNT and PCurrSettings != &EffSettings[Msg.Value]) {
                    PCurrSettings = &EffSettings[Msg.Value];
                    CrystalLeds::On();
                }
                break;

            case evtIdEverySecond:
//                Printf("Second\r");
                Iwdg::Reload();
                if(CrystalLeds::AreOff()) {
                    if(RadioTimeout_s > 0) RadioTimeout_s--;
                    else EnterSleep();
                }
                break;

            default: break;
        } // switch
    } // while true
}

/*
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
*/

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
            BKP->DR1 = N;
            if(N <= 7) {
                PCurrSettings = &EffSettings[N];
                PShell->Ok();
            }
            else PShell->BadParam();
        }
        else PShell->BadParam();
    }

    else if(PCmd->NameIs("HSM")) {
        uint32_t MinBrt, Smooth1, Smooth2;
        Color_t Clr;
        if(         PCmd->GetClrRGB(&Clr) == retvOk
                and PCmd->GetNext(&MinBrt) == retvOk
                and PCmd->GetNext(&Smooth1) == retvOk
                and PCmd->GetNext(&Smooth2) == retvOk
                ) {
            PCurrSettings->Clr = Clr;
            PCurrSettings->MinBrt = MinBrt;
            PCurrSettings->SmoothMin = Smooth1;
            PCurrSettings->SmoothMax = Smooth2;
            PShell->Ok();
        }
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
