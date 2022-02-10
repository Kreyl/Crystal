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

#endif

#if 1 // === ADC ===
extern Adc_t Adc;
void OnMeasurementDone();
TmrKL_t TmrOneS {TIME_MS2I(999), evtIdEverySecond, tktPeriodic};
uint32_t TimeS = 0;
#endif

#if 1 // ======================== LEDs related =================================
#define S_IN_MINUTE     60UL
#define SMOOTH          720UL
// Range of random colors
#define CLR_HSV_FROM    120
#define CLR_HSV_TO      180

LedRGBChunk_t lsqOn[] = {
        {csSetup, SMOOTH, clGreen},
        {csEnd}
};
LedRGBChunk_t lsqOff[] = {
        {csSetup, SMOOTH, clBlack},
        {csEnd}
};
CrystalLed_t Leds;

static uint32_t next = 1;
uint32_t MyRand() {
    next = next * 1103515245 + 12345;
    return (next / 65536) % 32768;
}

void StartNewColor() {
    uint32_t last = MyRand();
    uint16_t H = (last % (CLR_HSV_TO + 1 - CLR_HSV_FROM)) + CLR_HSV_FROM;
    Printf("%d\r", H);
    lsqOn[0].Color.FromHSV(H, 100, 100);
    Leds.StartOrRestart(lsqOn);
}
#endif

int main(void) {
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

    // LEDs
    Leds.Init();
    Leds.SetupSeqEndEvt(evtIdLedsDone);
    StartNewColor();

    SimpleSensors::Init(); // Buttons

    // Battery measurement
    PinSetupAnalog(ADC_BAT_PIN);
    PinSetupOut(ADC_BAT_EN, omPushPull);
    PinSetHi(ADC_BAT_EN);
    Adc.Init();

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

            case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                if(Msg.BtnEvtInfo.Type == beRelease) {
                    Adc.StartMeasurement();
                }
                break;

            case evtIdAdcRslt: OnMeasurementDone(); break;

            case evtIdEverySecond:
                TimeS++;
                if(TimeS < (S_IN_MINUTE * 15UL)) Leds.StartOrContinue(lsqOn);
                else Leds.StartOrContinue(lsqOff);
                break;

            case evtIdLedsDone:
                if(TimeS < (S_IN_MINUTE * 15UL)) StartNewColor();
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
    Leds.Stop();
    chThdSleepMilliseconds(108);
    ColorHSV_t hsv;
    if     (Percent <= 20) hsv = {0,   100, 100};
    else if(Percent <  80) hsv = {30,  100, 100};
    else                   hsv = {120, 100, 100};
    Leds.SetAllHsv(hsv);
    chThdSleepMilliseconds(1530);
    Leds.Stop();
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
