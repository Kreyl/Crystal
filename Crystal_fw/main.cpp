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
#endif

LedRGBChunk_t lsqIdle[] = {
        {csSetup, 360, clGreen},
        {csEnd}
};

LedRGBChunk_t lsqButton[] = {
        {csSetup, 0, clRed},
        {csWait, 4500},
        {csEnd}
};

uint32_t StateClrBtn = 0;

ColorHSV_t hsv(120, 100, 100);
TmrKL_t TmrSave {TIME_MS2I(3600), evtIdTimeToSave, tktOneShot};
CrystalLed_t Leds;

int main(void) {
    Clk.UpdateFreqValues();
    // Init OS
    halInit();
    chSysInit();
    OsIsInitialized = true;
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init();
    // Remap pins: disable JTAG leaving SWD, T3C2 at PB5, T2C3&4 at PB10&11, USART1 at PB6/7
    AFIO->MAPR = (0b010UL << 24) | (0b10UL << 10) | (0b10UL << 8) | AFIO_MAPR_USART1_REMAP;
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    // LEDs
    Leds.Init();
    EvtMsg_t msg;
    msg.ID = evtIdLedsDone;
    Leds.SetupSeqEndEvt(msg);
    Leds.StartOrRestart(lsqIdle);

    SimpleSensors::Init(); // Buttons

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
                if(StateClrBtn == 0) {
                    lsqButton[0].Color = clRed;
                    StateClrBtn = 1;
                }
                else if(StateClrBtn == 1) {
                    lsqButton[0].Color = clGreen;
                    StateClrBtn = 2;
                }
                else {
                    lsqButton[0].Color = clBlue;
                    StateClrBtn = 0;
                }
                Leds.StartOrRestart(lsqButton);
                break;

            case evtIdLedsDone:
//                Printf("Done\r");
                hsv.H = Random::Generate(0, 360);
                lsqIdle[0].Color = hsv.ToRGB();
                lsqIdle[0].Time_ms = Random::Generate(135, 450);
                Leds.StartOrRestart(lsqIdle);
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
