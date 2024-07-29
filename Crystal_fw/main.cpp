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

extern Adc_t Adc;
void OnMeasurementDone();

static const uint32_t kEffCnt = 2;

const EffSettings eff_settings[kEffCnt] = {
     //  Off     On       Smooth     Color1 H    Color2 H
        {9, 45,  99,  99,  108, 810,    0,   2,   359, 360}, // 0 Red
        {7, 18,  45, 180,  720, 999,   80, 160,   200, 260}, // 1 Green&Blue
        /*
        {9, 45,  9, 54,   108, 360,   330, 360,   215, 230}, // 0 Requiem
        {9, 45,  9, 54,   108, 360,     0,  15,   250, 270}, // 2 tango
        {9, 45,  9, 54,   405, 630,    80, 160,   260, 290}, // 3 waltz
        {9, 45, 99, 99,   108, 630,   225, 250,   250, 265}, // 5 dance
        {9, 45,  9, 54,   270, 405,   120, 270,    50,  77}, // 6
        */
};

const EffSettings *pcurr_settings = &eff_settings[1];

TmrKL_t TmrOneS {TIME_MS2I(999), evtIdEverySecond, tktPeriodic};
#endif

void main(void) {
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
    // Power-on, or radio pkt received => proceed with init
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    // LEDs
    CrystalLeds::Init();
    CrystalLeds::On();
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
                if(Msg.BtnEvtInfo.BtnID == 0) {
                    Adc.StartMeasurement();
                }
                break;

            case evtIdRadioCmd:
                Printf("RCmd: %u\r", Msg.Value);
                CrystalLeds::On();
                if(Msg.Value == 0) pcurr_settings = &eff_settings[0];
                else pcurr_settings = &eff_settings[1];
                break;

            case evtIdEverySecond:
//                Printf("Second\r");
                Iwdg::Reload();
                break;

            case evtIdAdcRslt:
                OnMeasurementDone();
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
    CrystalLeds::On();
}

#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ok();
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else PShell->CmdUnknown();
}
#endif
