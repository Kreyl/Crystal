/*
 * board.h
 *
 *  Created on: 05 08 2018
 *      Author: Kreyl
 */

#pragma once

// ==== General ====
#define APP_NAME            "CrystalTimer"

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

// Version of PCB
#define PCB_VER                 1

// MCU type as defined in the ST header.
#define STM32F100xB

// Freq of external crystal if any. Leave it here even if not used.
#define CRYSTAL_FREQ_HZ         12000000
#define LSI_FREQ_HZ             40000 // See datasheet

// OS timer settings
#define STM32_ST_IRQ_PRIORITY   2
#define STM32_ST_USE_TIMER      15
#define SYS_TIM_CLK             (Clk.APB2FreqHz)    // Timer 15 is clocked by APB2

//  Periphery
#define I2C1_ENABLED            FALSE
#define SIMPLESENSORS_ENABLED   TRUE
#define BUTTONS_ENABLED         TRUE

#define ADC_REQUIRED            TRUE
#define STM32_DMA_REQUIRED      TRUE    // Leave this macro name for OS

#if 1 // ========================== GPIO =======================================
// EXTI
#define INDIVIDUAL_EXTI_IRQ_REQUIRED    FALSE

// Buttons
#define BTN1_PIN        GPIOA,  0
#define BTN2_PIN        GPIOB, 12, pudPullDown
#define BTN3_PIN        GPIOB, 13, pudPullDown

// LEDs
#define LED1_R          { GPIOB,  0,  TIM3, 3, invNotInverted, omPushPull, 255 }
#define LED1_G          { GPIOB,  1,  TIM3, 4, invNotInverted, omPushPull, 255 }
#define LED1_B          { GPIOA,  1,  TIM2, 2, invNotInverted, omPushPull, 255 }

#define LED2_R          { GPIOB, 11,  TIM2, 4, invNotInverted, omPushPull, 255 }
#define LED2_G          { GPIOA,  8,  TIM1, 1, invNotInverted, omPushPull, 255 }
#define LED2_B          { GPIOB, 10,  TIM2, 3, invNotInverted, omPushPull, 255 }

#define LED3_R          { GPIOA, 10,  TIM1, 3, invNotInverted, omPushPull, 255 }
#define LED3_G          { GPIOA, 11,  TIM1, 4, invNotInverted, omPushPull, 255 }
#define LED3_B          { GPIOA,  9,  TIM1, 2, invNotInverted, omPushPull, 255 }

#define LED4_R          { GPIOB,  8, TIM16, 1, invNotInverted, omPushPull, 255 }
#define LED4_G          { GPIOB,  9, TIM17, 1, invNotInverted, omPushPull, 255 }
#define LED4_B          { GPIOB,  5,  TIM3, 2, invNotInverted, omPushPull, 255 }

// ADC
#define ADC_BAT_EN      GPIOA, 2
#define ADC_BAT_PIN     GPIOA, 3

// UART
#define UART_GPIO       GPIOB
#define UART_TX_PIN     6
#define UART_RX_PIN     7

// Radio: SPI, PGpio, Sck, Miso, Mosi, Cs, Gdo0
#define CC_Setup0       SPI1, GPIOA, 5,6,7, GPIOA,4, GPIOB,2

#endif // GPIO

#if 1 // =========================== I2C =======================================
// i2cclkPCLK1, i2cclkSYSCLK, i2cclkHSI
#define I2C_CLK_SRC     i2cclkHSI
#define I2C_BAUDRATE_HZ 400000
#endif

#if ADC_REQUIRED // ======================= Inner ADC ==========================
// Clock divider: clock is generated from the APB2
#define ADC_CLK_DIVIDER		adcDiv2

// ADC channels
#define ADC_BATTERY_CHNL 	3
#define ADC_VREFINT_CHNL    17 // const, see RefMan p.161
#define ADC_CHANNELS        { ADC_BATTERY_CHNL, ADC_VREFINT_CHNL }
#define ADC_CHANNEL_CNT     2   // Do not use countof(AdcChannels) as preprocessor does not know what is countof => cannot check
#define ADC_SAMPLE_TIME     ADC_SampleTime_55_5Cycles
#define ADC_SAMPLE_CNT      8   // How many times to measure every channel
#define ADC_SEQ_LEN         (ADC_SAMPLE_CNT * ADC_CHANNEL_CNT)
#endif

#if 1 // =========================== DMA =======================================
// ==== Uart ====
// Remap is made automatically if required
#define UART_DMA_TX     STM32_DMA_STREAM_ID(1, 4)
#define UART_DMA_RX     STM32_DMA_STREAM_ID(1, 5)
#define UART_DMA_CHNL   0 // dummy
#define UART_DMA_TX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_LOW | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_TCIE)
#define UART_DMA_RX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_MEDIUM | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC)

#if ADC_REQUIRED
#define ADC_DMA         STM32_DMA_STREAM_ID(1, 1)
#define ADC_DMA_MODE    STM32_DMA_CR_CHSEL(0) |   /* dummy */ \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */
#endif // ADC


#endif // DMA

#if 1 // ========================== USART ======================================
#define PRINTF_FLOAT_EN FALSE
#define UART_TXBUF_SZ   128
#define UART_RXBUF_SZ   84
#define CMD_BUF_SZ      64

#define CMD_UART        USART1

#define UARTS_CNT       1

#define CMD_UART_PARAMS \
    CMD_UART, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN, \
    UART_DMA_TX, UART_DMA_RX, UART_DMA_TX_MODE(UART_DMA_CHNL), UART_DMA_RX_MODE(UART_DMA_CHNL)

#endif
