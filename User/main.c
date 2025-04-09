/*
********************************* (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2022/08/08
 * Description        : Main program body.
 ******************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * This software (modified or not) and binary are used for WCH MPU
 ******************************************************************************
 */

/// MPU: CH32V003F4P6
/// Function: SPWM-TIM1-DMA, ILI9341-SPI-DMA, ADC1-CH7-DMA
/// Author: KY Lee

#include "debug.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

///-|----------------------|----------|--------------------|
/// | Data TRansfer Source |  DMA-CH  | Destination        | 
///-|----------------------|----------|--------------------|
/// | 62 Step SPWM Table   | DMA1-CH5 | TIM1-CH1, TIM1-CH2 | 
///-|----------------------|----------|--------------------|
/// | 10 Bit ADC1-CH7 data | DMA1-CH1 | ADC-BUFF (Average) | 
///-|----------------======|----------|--------------------|
/// | 320x240 Graphic data | DMA1-CH3 | SPI-ILI9341        |
///-|----------------------|----------|--------------------|

#include "ILI9341.h"
///---------------------------------------------------------------|
/// | CH32V003 Port  | ILI9341 Pin | LCD Description              |
///-|----------------|------------|-------------------------------|
/// |                | 1 - VDD    | 3V3                           |
/// |                | 2 - GND    | GND                           |
/// | PC4            | 3 - CS     | SPI SS (Chip Select)          |
/// |                | 4 - RESET  | 3V3 (/Reset)                  |
/// | PC3            | 5 - DC     | DC (Data, /Command)           |
/// | PC6 (SPI MOSI) | 6 - SDA    | MOSI (Master Output Slave In) |
/// | PC5 (SPI SCLK) | 7 - SCK    | SCLK (Serial Clock)           |
/// |                | 8 - LED    | 3V3  (Back Light)             |
///---------------------------------------------------------------|

// Define ILI9341
#define ILI9341_WIDTH    320
#define ILI9341_HEIGHT   240

//--------------------------------------------------------
// TIM1 CCPx register Definition
//--------------------------------------------------------
#define TIM1_CH1CVR_ADDRESS 0x40012C34  // CCR1 for CH1
#define TIM1_CH2CVR_ADDRESS 0x40012C38  // CCR2 for CH2
#define TIM1_CH3CVR_ADDRESS 0x40012C3C  // CCR3 for CH3
#define TIM1_CH4CVR_ADDRESS 0x40012C40  // CCR4 for CH4

/* Private variables */
// 48MHz /(16 *200 *62 *2) = 120Hz
#define TIM1_PSC    16   // Clock = 6MHz, SPWM = 15KHz
#define TIM1_ARR    201  // Amplitude = 0~200

#define buf_size 62    // Sine step =62 (180deg)
// Sine amplitude = 0~200 (reference =50%)
u16 sine_tbl[buf_size] = {
      5, 10, 15, 20, 25, 30, 34, 39, 44, 48,
     52, 57, 61, 65, 68, 72, 75, 79, 83, 85,
     87, 89, 91, 93, 95, 96, 97, 99, 99,100,
    100,100, 99, 99, 97, 96, 95, 93, 91, 89,
     87, 85, 83, 79, 75, 72, 68, 65, 61, 57,
     52, 48, 44, 39, 34, 30, 25, 20, 15, 10, 
      5,  0
};

// sine amplitude = 0~200 (feedback =output)
u16 sine_fdb[buf_size] = {
      5, 10, 15, 20, 25, 30, 34, 39, 44, 48,
     52, 57, 61, 65, 68, 72, 75, 79, 83, 85,
     87, 89, 91, 93, 95, 96, 97, 99, 99,100,
    100,100, 99, 99, 97, 96, 95, 93, 91, 89,
     87, 85, 83, 79, 75, 72, 68, 65, 61, 57,
     52, 48, 44, 39, 34, 30, 25, 20, 15, 10, 
      5,  0
};

//--------------------------------------------------------
// Port of the Sine PWM
//--------------------------------------------------------
// PD2 =CH1 (0~180deg High Side)
// PD0 =CH1N (0~180deg LOw Side)
// PA1 =CH2 (180~360deg High Side)
// PA2 =CH2N (180~360deg LOwh Side)
// PC2 =BRKIN (High for OCP)

// PC1 =LED for DMA Transfer Status
//--------------------------------------------------------

#define DMA_LED GPIO_Pin_1      // PC1
#define TIM1_BKIN GPIO_Pin_2    // PC2

#define TIM1_CH1 GPIO_Pin_2     // PD2
#define TIM1_CH1N GPIO_Pin_0    // PD0

#define TIM1_CH2 GPIO_Pin_1     // PA1
#define TIM1_CH2N GPIO_Pin_2    // PA2

//--------------------------------------------------------
// TIM1 init
// Timer DMA routines: TIM1_CH1(PD2) + TIM1_Dead_Time_Init
// PWM output use DMA to TIM1_CH1(CH1 =PD2, CH1N =PD0)
// (complementary output and dead time)
//--------------------------------------------------------
void TIM1_PWMOut_Init(u16 arr, u16 psc, u16 ccp)
{
    GPIO_InitTypeDef        GPIO_InitStructure = {0};
    TIM_OCInitTypeDef       TIM_OCInitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    TIM_BDTRInitTypeDef     TIM_BDTRInitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // DMA LED =PC1, TIM1 BRKIN =PC2
    GPIO_InitStructure.GPIO_Pin = DMA_LED;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_SetBits(GPIOC, DMA_LED);   // DMA_LED =ON

    // TIM1 BRKIN =PC2
    GPIO_InitStructure.GPIO_Pin = TIM1_BKIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // CH1N =PD0, CH1 =PD2,
    GPIO_InitStructure.GPIO_Pin = TIM1_CH1N | TIM1_CH1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // TIM1 CH2 =PA1, CH2N =PA2
    GPIO_InitStructure.GPIO_Pin = TIM1_CH2 | TIM1_CH2N;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // TIM1 Clock
    TIM_DeInit(TIM1);
    TIM_TimeBaseInitStructure.TIM_Period = arr -1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = psc -1;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

    // TIM1 CH1 Output =CH1 + CH1N =PD2, PD0
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;   
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;

    TIM_OCInitStructure.TIM_Pulse = ccp;  // start duty =50%
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; 
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;  
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);

    // TIM1 BRKIN =PC2, TIM1_Dead_Time
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Disable;
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Disable;
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF;

    TIM_BDTRInitStructure.TIM_DeadTime = 200;   // dead time =20us
    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable; // external break =PC2
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);   // Enable PWM output
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Disable);

    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE); //  Start TIM1
}

//--------------------------------------------------------
// TIM1_DMA_Init
// Initializes the TIM DMAy Channelx configuration.
//--------------------------------------------------------
void TIM1_DMA_Init(DMA_Channel_TypeDef *DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize)
{
    DMA_InitTypeDef DMA_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA_CHx);
    DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
    DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = bufsize;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;

    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    //DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;

    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA_CHx, &DMA_InitStructure);
    DMA_Cmd(DMA_CHx, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE );
}

//--------------------------------------------------------
// interrupt for End of DMA Transfer
//--------------------------------------------------------
void DMA1_Channel5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel5_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC5) != RESET )
    {
        if (GPIO_ReadOutputDataBit(GPIOC, DMA_LED) ==SET)
        {
            GPIO_ResetBits(GPIOC, DMA_LED);  // DMA LED off

            // 180~360deg Sine PWM by Sine Table
            TIM1_DMA_Init(DMA1_Channel5, (u32)TIM1_CH2CVR_ADDRESS, (u32)sine_fdb, buf_size);
        }

        else
        {
            GPIO_SetBits(GPIOC, DMA_LED);   // DMA LED on

            // 0~180deg Sine PWM by Sine Table
            TIM1_DMA_Init(DMA1_Channel5, (u32)TIM1_CH1CVR_ADDRESS, (u32)sine_fdb, buf_size);
        }
        DMA_ClearITPendingBit(DMA1_IT_TC5);
    }
}

//--------------------------------------------------------
// TIM2_INT_Init(USER_DELAY -1, 48 -1);
// TIM2 Clock =1us
//--------------------------------------------------------
static u8 timer2_flag =1;

void TIM2_INT_Init(u32 arr, u16 psc)
{
    TIM_TimeBaseInitTypeDef TIMBase_InitStruct;
    //TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStruct;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_DeInit(TIM2);
    TIMBase_InitStruct.TIM_Period = arr -1;
    TIMBase_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIMBase_InitStruct.TIM_Prescaler = psc -1;
    TIMBase_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIMBase_InitStruct);

    //TIM_OCInitStructure.TIM_Pulse = 0;
    //TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 5;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    TIM_Cmd(TIM2, ENABLE);  //  Start TIM2

    timer2_flag =1; // set timer2 flag
}

//---------------------------------------------------------------------
// TIM1_IRQHandler handles of TIM1 interrupt
//---------------------------------------------------------------------
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void)
{
   if( TIM_GetITStatus(TIM1, TIM_IT_Update ) ==SET)
   {
       //printf( "TIM1\r\n" );
   }
   // Clear TIM1 flag
   TIM_ClearITPendingBit(TIM1, TIM_IT_Update );
}

//---------------------------------------------------------------------
// TIM2_IRQHandler handles of TIM2 interrupt
// Interrupt flag is set
//---------------------------------------------------------------------
void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void)
{
   if (TIM_GetITStatus(TIM2, TIM_IT_Update) ==SET)
   {
       TIM_Cmd(TIM2, DISABLE);  // Stop TIM2

       // this can be replaced with your code of flag set
       // so that in main's that flag can be handled
       timer2_flag =0;  // end of timer2

       // Clear TIM2 flag
       TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
   }
}

//---------------------------------------------------------------------
// Init ADC1_CH7 =PD4 (10 Bit ADC)
//---------------------------------------------------------------------
void init_ADC(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    ADC_InitTypeDef  ADC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    // ADC_CH7 =PD4
    ADC_DeInit(ADC1);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // ADC1 Single channel use internal clock
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    // Calibration voltage
    ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);
    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    // Reset Calibration
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    
    // Start Calibrartion
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

//---------------------------------------------------------------------
// Initializes the DMAy Channelx configuration.
// DMA_CHx - x can be 1 to 7.
// ppadr - Peripheral base address.
// memadr - Memory base address.
// bufsize - DMA channel buffer size.
//---------------------------------------------------------------------
void DMA_Tx_Init(DMA_Channel_TypeDef *DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize)
{
    DMA_InitTypeDef DMA_InitStructure = {0};
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA_CHx);
    DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
    DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = bufsize;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA_CHx, &DMA_InitStructure);
}

//---------------------------------------------------------------------
// Init DMA1-CH1 for ADC value
//---------------------------------------------------------------------
#define adc_buf_size (10)   // Number of average =10
u16 adc_BUF[adc_buf_size +1];

void init_DMA(void)
{
    // Init DMAx for ADC1
    DMA_Tx_Init(DMA1_Channel1, (u32)&ADC1->RDATAR, (u32)adc_BUF, adc_buf_size);
    DMA_Cmd(DMA1_Channel1, ENABLE); // Start DMA1_CH1

    // Start ADC1_CH7
    ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 1, ADC_SampleTime_30Cycles);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);     // Start

    //Delay_Ms(50);
    //ADC_SoftwareStartConvCmd(ADC1, DISABLE);    // Stop
}

//---------------------------------------------------------------------
// Single Read ADC1-CH7 (not use DMA)
//---------------------------------------------------------------------
u16 get_ADC(void)
{
    u16 val;    // 10 bit result of ADC1_CH7

    ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 1, ADC_SampleTime_30Cycles);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));  // End of ADC?
    val = ADC_GetConversionValue(ADC1); // Read ADC vlaue
    return val;
}

//---------------------------------------------------------------------
// Display ADC-CH7 0~1023
//---------------------------------------------------------------------
u32 adc_val;
u32 mv_val;
char dec_str[9];

void disp_ADC(void)
{
    u16 i;
    u16 adc_sum =0;
    u16 ave_val;

    // read ADC buffer from DMA1
    for(i = 0; i < adc_buf_size; i++)
    {
        //adc_sum += (u32)(get_ADC());    // read direct ADC1-CH7
        adc_sum += adc_BUF[i];  // make sum by DMA1-CH1 tranfered
    }
    ave_val =adc_sum /adc_buf_size; // make average of ADC

    // Start ADC1-CH7 data DMA transfer to adc_BUF 
    DMA_Tx_Init(DMA1_Channel1, (u32)&ADC1->RDATAR, (u32)adc_BUF, adc_buf_size);
    DMA_Cmd(DMA1_Channel1, ENABLE); // Start DMA1_CH1

    adc_val =(u32)(ave_val);    // save for the feedback control
    mv_val =(ave_val *3250) /1023; // make [mV] from measured VCC value =3.25V

    tft_set_cursor(0, 98);
    tft_set_color(GREEN);
    tft_print("ADC1-CH7: ");

    // binary to 4 digit decimal as right align
    sprintf(dec_str, "%4d", mv_val);
    tft_set_cursor(54, 98);
    tft_set_color(YELLOW);
    tft_print(dec_str);

    // display unit =mV
    tft_set_cursor(84, 98);
    tft_set_color(GREEN);
    tft_print("mV");
}

//---------------------------------------------------------------------
// Display User Timer2 Counter (0~9999)
//---------------------------------------------------------------------
u32 timer2_cnt =0;  // Start user time =0ms

void disp_TIM2(void)
{
    tft_set_cursor(0, 106);
    tft_set_color(GREEN);
    tft_print("TIM2-CNT: ");

    // Read TIM2-CNT value [ms]
    timer2_cnt = TIM2->CNT;

    // 32 bit binary to 4 digit decimal as right align
    sprintf(dec_str, "%4d", timer2_cnt);
    tft_set_cursor(54, 106);
    tft_set_color(CYAN);
    tft_print(dec_str);

    // Display unit =[ms]
    tft_set_cursor(84, 106);
    tft_set_color(GREEN);
    tft_print("ms");
}

//---------------------------------------------------------------------
// Display Main Menu at ST7789 (128x160)
//---------------------------------------------------------------------
void disp_MENU(void)
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);
    tft_set_background_color(BLACK);
    tft_set_color(GREEN);
    tft_set_cursor(0, 1);
    tft_print("CH32V003 SPWM & ILI9341 Demo");

    tft_set_color(WHITE);
    tft_set_cursor(0, 16);
    tft_print("1. Random dot");
    tft_set_cursor(0, 24);
    tft_print("2. Horizon line");
    tft_set_cursor(0, 32);
    tft_print("3. Vertical line");
    tft_set_cursor(0, 40);
    tft_print("4. Random line");
    tft_set_cursor(0, 48);
    tft_print("5. Center Rectangle");
    tft_set_cursor(0, 56);
    tft_print("6. Random Rectangle");
    tft_set_cursor(0, 64);
    tft_print("7. Fill rectangle");
    tft_set_cursor(0, 72);
    tft_print("8. Move Rectangle");
    tft_set_cursor(0, 80);
    tft_print("9. Random Circle");
}

//---------------------------------------------------------------------
// White Noise Generator State
//---------------------------------------------------------------------
#define NOISE_BITS      8
#define NOISE_MASK      ((1 << NOISE_BITS) -1)
#define NOISE_POLY_TAP0 31
#define NOISE_POLY_TAP1 11
#define NOISE_POLY_TAP2 1
#define NOISE_POLY_TAP3 0

uint32_t frame = 0;
uint16_t colors[] =
{
    BLACK, NAVY, DARKGREEN, DARKCYAN, MAROON,
    PURPLE, OLIVE, LIGHTGREY, DARKGREY, BLUE,
    GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE,
    ORANGE, GREENYELLOW, PINK,
};

//---------------------------------------------------------------------
// random byte generator
//---------------------------------------------------------------------
uint32_t lfsr = 1;

uint16_t rand16(void)
{
    u16  bit;
    u32 new_data;

    for (bit = 0; bit < NOISE_BITS; bit++)
    {
        new_data = ((lfsr >> NOISE_POLY_TAP0) ^ (lfsr >> NOISE_POLY_TAP1) ^ (lfsr >> NOISE_POLY_TAP2) ^
                    (lfsr >> NOISE_POLY_TAP3));
        lfsr     = (lfsr << 1) | (new_data & 1);
    }
    return (lfsr & NOISE_MASK) *3;
}

//---------------------------------------------------------------------
// draw random Dot
//---------------------------------------------------------------------
void random_dot()
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    // user interval timer =TIM2 clock =1ms
    TIM2_INT_Init(1000, 48000);   // ARR =1sec
    while(timer2_flag)
    {
        for (uint16_t i = 0; i < ILI9341_WIDTH; i++)
        {
            u16 c =colors[rand16() %19];
            tft_draw_pixel(rand16() %ILI9341_WIDTH, rand16() %ILI9341_HEIGHT, c);
            tft_draw_pixel((rand16() %ILI9341_WIDTH) +1, (rand16() %ILI9341_HEIGHT), c);
            tft_draw_pixel((rand16() %ILI9341_WIDTH), (rand16() %ILI9341_HEIGHT) +1, c);
            tft_draw_pixel((rand16() %ILI9341_WIDTH) +1, (rand16() %ILI9341_HEIGHT) +1, c);
        }
    }
}

//---------------------------------------------------------------------
// Scan H-Line
//---------------------------------------------------------------------
void scan_hline()
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    // user interval timer =TIM2 clock =1ms
    TIM2_INT_Init(1000, 48000);   // ARR =1sec
    while(timer2_flag)
    {
        for (uint16_t i = 0; i < ILI9341_WIDTH; i++)
        {
            tft_draw_line(0, i, ILI9341_WIDTH, i, colors[rand16() %19]);
        }
    }
}

//---------------------------------------------------------------------
// Scan V-Line
//---------------------------------------------------------------------
void scan_vline()
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    // user interval timer =TIM2 clock =1ms
    TIM2_INT_Init(1000, 48000);   // ARR =1sec
    while(timer2_flag)
    {
        for (uint16_t i = 0; i < ILI9341_WIDTH; i++)
        {
            tft_draw_line(i, 0, i, ILI9341_HEIGHT, colors[rand16() %19]);
        }
    }
}

//---------------------------------------------------------------------
// Random Line
//---------------------------------------------------------------------
void random_line(void)
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    // user interval timer =TIM2 clock =1ms
    TIM2_INT_Init(1000, 48000);   // ARR =1sec
    while(timer2_flag)
    {
        tft_draw_line(rand16() %ILI9341_WIDTH, rand16() %ILI9341_HEIGHT, rand16() %ILI9341_WIDTH, rand16() %ILI9341_HEIGHT, colors[rand16() %19]);
    }
}

//---------------------------------------------------------------------
// Centered Rectangle
//---------------------------------------------------------------------
void center_rect(void)
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    // user interval timer =TIM2 clock =1ms
    TIM2_INT_Init(1000, 48000);   // ARR =1sec
    while(timer2_flag)
    {
        for (uint8_t i = 0; i < 120; i++)
        {
            tft_draw_rect(i, i, ILI9341_WIDTH -(i << 1), ILI9341_HEIGHT -(i << 1), colors[rand16() %19]);
        }
    }
}

//---------------------------------------------------------------------
// Random Rectangle
//---------------------------------------------------------------------
void random_rect(void)
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    // user interval timer =TIM2 clock =1ms
    TIM2_INT_Init(1000, 48000);   // ARR =1sec
    while(timer2_flag)
    {
        for (uint8_t i = 0; i < 120; i++)
        {
            tft_draw_rect(rand16() %ILI9341_WIDTH, rand16() %ILI9341_HEIGHT, 20, 20, colors[rand16() % 19]);
        }
    }
}

//---------------------------------------------------------------------
// Fill Rectangle
//---------------------------------------------------------------------
void fill_rect(void)
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    // user interval timer =TIM2 clock =1ms
    TIM2_INT_Init(1000, 48000);   // ARR =1sec
    while(timer2_flag)
    {
        for (uint8_t i = 0; i < 120; i++)
        {
            tft_fill_rect(rand16() %ILI9341_WIDTH, rand16() %ILI9341_HEIGHT, 20, 20, colors[rand16() %19]);
        }
    }
}

//---------------------------------------------------------------------
// Move Rectangle
//---------------------------------------------------------------------
void move_rect(void)
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    frame =1000;
    u16 x =0, y =0, step_x =2, step_y =2;
    while (frame-- >0)
    {
        uint16_t bg = colors[rand16() %19];
        tft_fill_rect(x, y, 40, 20, bg);
        tft_set_color(colors[rand16() %19]);
        
        x += step_x;
        if (x >= ILI9341_WIDTH -40)
        {
            step_x = -step_x;
        }
        y += step_y;
        if (y >= ILI9341_HEIGHT -20)
        {
            step_y = -step_y;
        }
    }
}

//---------------------------------------------------------------------
// Random Circle
//---------------------------------------------------------------------
void random_circ(void)
{
    tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);

    // user interval timer =TIM2 clock =1ms
    TIM2_INT_Init(1000, 48000);   // ARR =1sec
    while(timer2_flag)
    {
        for (uint8_t i = 0; i < 80; i++)
        {
            tft_draw_circle(rand16() %ILI9341_WIDTH, rand16() %ILI9341_HEIGHT, 10, colors[rand16() % 19]);
        }
    }
}

//---------------------------------------------------------------------
// ST7735 demo display
//---------------------------------------------------------------------
void demo_LCD(void)
{
    random_dot();

    scan_hline();
    scan_vline();
    random_line();

    center_rect();
    random_rect();

    fill_rect();
    move_rect();

    random_circ();
}

//---------------------------------------------------------------------
// Main program.
//---------------------------------------------------------------------
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();    // HSI 48MHz
    Delay_Init();

    // all clear AF of GPIO
    GPIO_AFIODeInit();

#if (SDI_PRINT == SDI_PR_OPEN)
    SDI_Printf_Enable();
#else
    USART_Printf_Init(115200);
#endif
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID() );

    // init SPWM waveform
    // (psc, arr*2 , ccp) for 15.0KHz PWM / 62 Step =120Hz
    TIM1_PWMOut_Init(TIM1_ARR, TIM1_PSC, 0);

    // Sine PWM to CH1, CH1N,
    TIM1_DMA_Init(DMA1_Channel5, (u32)TIM1_CH1CVR_ADDRESS, (u32)sine_fdb, buf_size);
    //TIM1_DMA_Init(DMA1_Channel5, (u32)TIM1_CH2CVR_ADDRESS, (u32)sine_fdb buf_size);

    TIM_DMACmd(TIM1, TIM_DMA_Update, ENABLE);   // Start TIM1_DMA
    TIM_Cmd(TIM1, ENABLE);  //  Start TIM1

    init_ADC();
    init_DMA();

    // Init ST7735 TFT LCD
    tft_init();
    Delay_Ms(100);

    // End of Hardware Setup
    while(1)
    {
        //tft_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, BLACK);
        disp_MENU();

        // user interval timer =TIM2 clock =1ms
        TIM2_INT_Init(5000, 48000);   // ARR =5sec
        while(timer2_flag)
        {
            // Dispaly ADC-CH7 (0~1023) and TIM2-CNT (0~9999)
            disp_ADC();     // Read ADC binary and decimal display [mV]
            disp_TIM2();    // Display timer2 [ms]

            Delay_Ms(25);   // Display time =25ms
        }
        
        demo_LCD();     // Display graphic demo
        // end user code
    }
}   // End of main()
//---------------------------------------------------------------------
