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

ColorHSV_t ordinal_clr = {300, 100, 100}; // Magenta
ColorHSV_t choosen_clr = {240, 100, 100}; // Blue, to start with red

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

            case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                if(Msg.BtnEvtInfo.BtnID == 0 and (Msg.BtnEvtInfo.Type == beShortPress or Msg.BtnEvtInfo.Type == beRepeat)) {
                    ordinal_clr.H++;
                    if(ordinal_clr.H >= CLR_HSV_H_MAX) ordinal_clr.H = 0;
                    CrystalLeds::SetHsvNow(ordinal_clr);
                }
                else if(Msg.BtnEvtInfo.BtnID == 1 and (Msg.BtnEvtInfo.Type == beShortPress or Msg.BtnEvtInfo.Type == beRepeat)) {
                    if(ordinal_clr.H == 0) ordinal_clr.H = CLR_HSV_H_MAX;
                    else ordinal_clr.H--;
                    CrystalLeds::SetHsvNow(ordinal_clr);
                }
                else if(Msg.BtnEvtInfo.BtnID == 2 and Msg.BtnEvtInfo.Type == beShortPress) { // Switch choosen color
                    if     (choosen_clr.H == 0)   choosen_clr.H = 120; // Red->Green
                    else if(choosen_clr.H == 120) choosen_clr.H = 240; // Green->Blue
                    else choosen_clr.H = 0;  // Blue (or whatever) ->Red
                    ordinal_clr = choosen_clr;
                    CrystalLeds::SetHsvSmoothly(ordinal_clr);
                }
                break;

            case evtIdRadioCmd:
                Printf("RCmd\r");
                choosen_clr.H = Msg.Value;
                ordinal_clr.H = Msg.Value;
                CrystalLeds::SetHsvSmoothly(ordinal_clr);
                break;

            case evtIdEverySecond:
//                Printf("Second\r");
                Iwdg::Reload();
                break;

            default: break;
        } // switch
    } // while true
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
