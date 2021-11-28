#if 1 // ============================ Includes =================================
#include "hal.h"
#include "MsgQ.h"
#include "uart.h"
#include "shell.h"
#include "led.h"
//#include "buttons.h"
//#include "Sequences.h"
#endif
#if 1 // ======================== Variables & prototypes =======================
// Forever
bool OsIsInitialized = false;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
void OnCmd(Shell_t *PShell);
void ITask();

//#define BATTERY_LOW_mv  3200
//#define BATTERY_DEAD_mv 3300

LedRGB_t Leds[4] = {
        {LED1_R, LED1_G, LED1_B},
        {LED2_R, LED2_G, LED2_B},
        {LED3_R, LED3_G, LED3_B},
        {LED4_R, LED4_G, LED4_B},
};

#endif

int main(void) {
#if 1 // ==== Iwdg, Clk, Os, EvtQ, Uart ====
    // Start Watchdog. Will reset in main thread by periodic 1 sec events.
//    Iwdg::InitAndStart(4500);
//    Iwdg::DisableInDebug();
//    // Setup clock frequency
//    Clk.SetCoreClk(cclk48MHz);
//    // 48MHz clock
//    Clk.SetupSai1Qas48MhzSrc();
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

    for(auto &Led : Leds) {
        Led.Init();
//        Led.SetColor(clGreen);
//        chThdSleepMilliseconds(270);
    }



//    Buttons::Init();
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

//            case evtIdPwrOffTimeout:
//                Printf("TmrOff timeout\r");
//                break;
/*
            case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnInfo.ID, Msg.BtnInfo.Evt);
                if(Msg.BtnInfo.Evt == bePress) {
                    Resume();
                    IsPlayingIntro = false;
                    AuPlayer.FadeOut();
                    Songs[Msg.BtnInfo.ID - 1].Play();
                }
                else if(Msg.BtnInfo.Evt == beRelease) {
                    if(IsPlayingIntro) IsPlayingIntro = false;
                    else if(Buttons::AreAllIdle()) AuPlayer.FadeOut();
                }
                else if(Msg.BtnInfo.Evt == beCombo) {
                    Resume();
                    IsPlayingIntro = true;
                    MustSleep = true;
                    AuPlayer.Play("Sleep.wav", spmSingle);
                }
                break;
*/
                /*
            case evtIdEverySecond:
//                Printf("Second\r");
//                Iwdg::Reload();
                // Check battery
                if(!IsStandby) {
                    uint32_t Battery_mV;
                    if(Codec.GetBatteryVmv(&Battery_mV) == retvOk) {
//                    Printf("%u\r", Battery_mV);
                        if(Battery_mV < BATTERY_DEAD_mv) {
                            Printf("Discharged: %u\r", Battery_mV);
                            EnterSleep();
                        }
                    }
                }
                break;
                */

            default: break;
        } // switch
    } // while true
}
/*
void Resume() {
    if(!IsStandby) return;
    Printf("Resume\r");
    // Clock
    Clk.SetCoreClk(cclk48MHz);
    Clk.SetupSai1Qas48MhzSrc();
    Clk.UpdateFreqValues();
    Clk.PrintFreqs();
    // Sound
    Codec.Init();
    Codec.SetSpeakerVolume(-96);    // To remove speaker pop at power on
    Codec.DisableHeadphones();
    Codec.EnableSpeakerMono();
    Codec.SetupMonoStereo(Stereo);  // For wav player
    Codec.SetupSampleRate(22050); // Just default, will be replaced when changed
    Codec.SetMasterVolume(5); // 12 is max
    Codec.SetSpeakerVolume(0); // 0 is max

    IsStandby = false;
}

void Standby() {
    Printf("Standby\r");
    // Sound
    Codec.Deinit();
    // Clock
    Clk.SwitchToMSI();
    Clk.DisablePLL();
    Clk.DisableSai1();

    Clk.UpdateFreqValues();
    Clk.PrintFreqs();
    IsStandby = true;
}

void EnterSleepNow() {
    // Enable inner pull-ups
//    PWR->PUCRC |= PWR_PUCRC_PC13;
    // Enable inner pull-downs
    PWR->PDCRA |= PWR_PDCRA_PA0 | PWR_PDCRA_PA2;
//    PWR->PDCRC |= PWR_PDCRC_PC13;
    // Apply PullUps and PullDowns
    PWR->CR3 |= PWR_CR3_APC;
    // Enable wake-up srcs
    Sleep::EnableWakeup1Pin(rfRising); // Btn1
//    Sleep::EnableWakeup2Pin(rfRising); // Btn2
    Sleep::EnableWakeup4Pin(rfRising); // USB
    Sleep::ClearWUFFlags();
    Sleep::EnterStandby();
}

void EnterSleep() {
    Printf("Entering sleep\r");
    chThdSleepMilliseconds(45);
    chSysLock();
    EnterSleepNow();
    chSysUnlock();
}
*/

#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ok();
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    else if(PCmd->NameIs("mem")) PrintMemoryInfo();

    else if(PCmd->NameIs("Clr")) {
        uint8_t N;
        Color_t Clr;
        if(PCmd->GetNext<uint8_t>(&N) == retvOk) {
            if(PCmd->GetClrRGB(&Clr) == retvOk) {
                Leds[N].SetColor(Clr);
            }
            else PShell->BadParam();
        }
        else PShell->BadParam();
    }


    else PShell->CmdUnknown();
}
#endif
