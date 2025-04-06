# CH32V003-ILI9341-SPWM-SPI-ADC-DMA
CH32V003F4P6 graphic demo display to ILI9341 with SPWM-DMA, SPI-DMA , ADC-DMA and Timer2 interrupt.

![CH32V003F4P6-GPIO-AF-Functions](https://github.com/user-attachments/assets/61384d33-e228-48b3-bc96-e78fcd676003)

62 step Sine PWM table used DMA1-CH5 transfer to TIM1-CH1 and TIM1-CH2, Timer 1 generate 4 PWM pulses for the single phase full brdige driver. 
Full bridge PWM driver can be make high voltage 60Hz sine wave output for the DC-AC inverter.

TIM1 can be continuous generating 4 PWM pulses at TIM1-CH1 (PD2), TIM1-CH1N (PD0), TIM1-CH2 (PA1) and TIM1-CH2N (PA2) with TIM1-BRKIN (PC2) for the OCP or any other protection. 

![CH32V003-TIM1-SPWM-Output](https://github.com/user-attachments/assets/47d447a1-c4d3-4fb7-a0d7-ed73b3ef16f5)

![SPWM-CH1-PD2-Wave](https://github.com/user-attachments/assets/84fca3a2-072d-4a03-95a8-447104a26f7f)

SPI-DMA can be demo graphic data DMA transfer to SPI for the display on 320x240 ILI9341 TFT LCD.
I bought 2.4 inch ILI9341 and just conneted 160x128 ST7735 LCD firmware driver. 
But ST7735 firmware working ILI9341 even display little bit different position on ILI9341 LCD. 

I was modify ST7735 library to ILI9341 library. ILI9341 library only different "Memory Data Access Control" code for Set rotation of tft_init() in ILI9341.c (just copy and modify all name ST7735 to ILI9341). Modify required only exchange the screen rotation code as below in ST7735.c library. 

write_command_8(ILI9341_MADCTL); // 0x36 =Memory Data Access Control.
write_data_8(ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR );

Some times, 2.4 inch ILI9341 back-light brightness less than 1.4 inch small LCD, I was exchange R5 at connected BL drive transistor collector of Q1 at bottom side LCD module, if R5 exchange resistor 8R2 to 1R0 or 2R2 then LCD brightness can be more high brightness by back light LED current to increase. 

![ILI9341-Random-Dot](https://github.com/user-attachments/assets/4d4aa6b3-bdc3-490a-95c0-3e03ef606440)

![ILI9341-Horz-Line](https://github.com/user-attachments/assets/06a96a8e-3c8e-4265-97c0-76d69c3ed1a5)

![ILI9341-Center-Rectangle](https://github.com/user-attachments/assets/d85a9248-5738-454f-bbc7-39fe61ea978d)

![ILI9341-random-rect](https://github.com/user-attachments/assets/1b0b513b-f278-4b9e-9b01-3864501d036b)

![ILI9341-Filled-Rect](https://github.com/user-attachments/assets/ab051933-9b15-46f3-ad67-aa119cf5c4b9)

Averaged 10 bits ADC1-CH7 (PD4) data used only display on the LCD screen for the monitoring analog voltage and SPWM duty (sine wave amplitude) feedback control. (not implimented yet).

TIM2 can be use msec timer for user delay timer as TIM2 interrupt service.
This timer used TIM2->CNT get timer2 counter value by "on the fly" and diaplay in main menu while 5 sec as end of timer2 (0 - 9999ms)
After 30sec, Start 8 kind of graphic display to the LCD screen.

![ILI9341-Main-Menu](https://github.com/user-attachments/assets/4b1da89a-0075-4441-9209-7c19105dc871)

All source code made from reference of sample code at CH32V003 EVT for TSSOP-20pin WCH-CH32V003F4P6.
This MPU has advance TIM1, TIM2, ADC, SPI, I2S and DMA functions compatible ARM STM32F030 series as standard HAL library code style for MounRiver Studio V2.1.0 (used dark theme same as vsCode)

![MounRiver-Studio-V2 10](https://github.com/user-attachments/assets/0e819b0c-e9fd-42f1-a830-544bfa1f55a2)

Flash tool used WCH-LinkE-Mini. (GND, SWD, VCC connected to GND, PD1, VCC of CH32V003F4P6).

![ch32v003f4u6-LinkE-Mini-pcb](https://github.com/user-attachments/assets/124ea225-775c-461f-a42c-3c8b96791429)

This tool, almost all WCH microcontrollers (CH5xx, CH6xx, CH32Fxxx, CH32Vxxx, CH32Xxxx, and CH32Lxxx) which have a factory-builtin bootloader (v2.x.x) can be flashed via USB. And this tool can be support DAP-Link at Keil-UV5 for standard ARM CPU flash to STM32Fxxx seriese.

If you require change mode WCH-LinkE to WCH-LinkDAP (WinUSB or HID), WCH-LinkE flash tool can be change WCH-Link mode as press Get and Select mode and Set button in WCH-Link Utilty (V2.40). If you require restore to WCH-LinkE mode, press reset button on the WCH-LinkE flash tool and set require mode using WCH-LinkE utilty.
