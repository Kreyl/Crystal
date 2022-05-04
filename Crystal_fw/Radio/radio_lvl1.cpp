/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"

cc1101_t CC(CC_Setup0);

//#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    10
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    9
#define DBG2_SET()  PinSetHi(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinSetLo(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

rLevel1_t Radio;
int8_t Rssi;

extern EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 128);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        CC.Recalibrate();
//        CC.Transmit(&PktTx, RPKT_LEN);
        uint8_t Rslt = CC.Receive(27, &PktRx, RPKT_LEN, &Rssi);
        if(Rslt == retvOk) {
            if(PktRx.Sign == 0xCA115EA1) {
                EvtMsg_t Msg(evtIdRadioCmd, 0, 0);
                Msg.R = PktRx.R;
                Msg.G = PktRx.G;
                Msg.B = PktRx.B;
                Msg.BtnIndx = PktRx.BtnIndx;
                EvtQMain.SendNowOrExit(Msg);
            }
//            Printf("Clr: %u %u %u; Btn: %u; Rssi: %d\r", PktRx.R, PktRx.G, PktRx.B, PktRx.BtnIndx, Rssi);
        }
        CC.PowerOff();
        chThdSleepMilliseconds(360);
    } // while true
}
#endif // task

uint8_t rLevel1_t::InitCC() {
    if(CCIsInitialized) return retvOk;
    if(CC.Init() == retvOk) {
        CC.SetPktSize(RPKT_LEN);
        CC.DoIdleAfterTx();
        CC.SetChannel(RCHNL_EACH_OTH);
        CC.SetTxPower(CC_Pwr0dBm);
        CC.SetBitrate(CCBitrate100k);
        CCIsInitialized = true;
        return retvOk;
    }
    else return retvFail;
}

uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

//    RMsgQ.Init();
    if(InitCC() == retvOk) {
//        CC.EnterPwrDown();
        Printf("CC init ok\r");
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}

uint8_t rLevel1_t::InitAndRxOnce() {
    if(InitCC() == retvOk) {
        CC.Recalibrate();
        uint8_t Rslt = CC.Receive(27, &PktRx, RPKT_LEN, &Rssi);
        CC.PowerOff();
        if(Rslt == retvOk and PktRx.Sign == 0xCA115EA1 and PktRx.BtnIndx < 7) return retvOk;
    }
    return retvFail;
}
