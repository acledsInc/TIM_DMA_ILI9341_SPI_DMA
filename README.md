# CH32V003-ILI9341-SPWM-SPI-ADC-DMA

CH32V003F4P6 graphic demo display to ILI9341 with SPWM-DMA, SPI-DMA , ADC-DMA and Timer2 interrupt.
![CH32V003F4P6-GPIO-AF-Functions](https://github.com/user-attachments/assets/fb520454-e994-4299-b591-385fbc6d2b6a)

62 step Sine PWM table used DMA1-CH5 transfer to TIM1-CH1 and TIM1-CH2. 
Timer 1 generate 4 PWM pulses for the single phase full brdige driver. 
Full bridge PWM driver can be make high voltage 60Hz sine wave output for the DC-AC inverter.

TIM1 required continuously generating 4 PWM pulses at TIM1-CH1 (PD2), TIM1-CH1N (PD0), TIM1-CH2 (PA1) and TIM1-CH2N (PA2) with TIM1-BRKIN (PC2) for the OCP or any other external control. 
For example TIM1-BKIN (PC2) can be use external SPWM outputs ON - OFF switch.
![CH32V003-TIM1-SPWM-Output](https://github.com/user-attachments/assets/35a19328-bc73-448f-ac94-3d13ce385cb1)
![SPWM-CH1-PD2-Wave](https://github.com/user-attachments/assets/0d7e0965-c4c8-4f8c-aff1-ae9d0269faf8)
![SPWM-CH1N-PD0-Wave](https://github.com/user-attachments/assets/d9065acb-9148-42ce-beda-7ac9b6fd3ad2)

We have 2 way of make 220Vac 60Hz SPWM inverter, One is 12Vdc to 220Vac used 60Hz low frequency transformer.
Another way is 400Vdc to 220Vac 60Hz with AC output LC filter without low frequency transformer.
This way require 12Vdc to isolated 400Vdc SMPS for automotive or 220Vac to 400Vdc isolated SMPS for home.  

12Vdc to 220Vac Single phase inverter schematic diagram. 
MPU SPWM outputs do not connect as direct to MOSFET, 3.3V PWM pulses must use IR2110S MOSFET driver. 
![12Vdc-to-220Vac-Inverter-Sch](https://github.com/user-attachments/assets/80db2007-cec6-440c-9fa3-4fa4feb9e03e)

I bought 2.8 inch ILI9341 TFT LCD, And it just test for ST7735 library, I found ST7735 library compatible to ILI9341 by chance. ST7735 library little bit different only rotated display and reversed x and y.

I was modify ST7735 library to ILI9341 library. 
ILI9341 library only different "Memory Data Access Control" code for Set rotation in ILI9341.c 
(just copy and modify all name ST7735 to ILI9341). 
Modify required only exchange the screen rotation code as below in ST7735.c library. 

write_command_8(ILI9341_MADCTL); // 0x36 =Memory Data Access Control.
write_data_8(ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR );

If your 2.8 inch ILI9341 back-light brightness less than 1.4 inch small LCD when use 3.3V VDD. 
You can be exchange R5 at connected BL drive transistor collector (Q1).
Resistor R5 exchange 8R2 to 1R0 or 2R2 then LCD brightness increase more high brightness. 
![ILI9324-240x320-v1 2-bottom](https://github.com/user-attachments/assets/ebcde002-1628-449e-a546-9111e52792f3)

Averaged 10 bits ADC1-CH7 (PD4) data used only display on the LCD screen for the monitoring analog voltage.
and SPWM duty (sine wave amplitude) feedback control. (not implimented yet).

TIM2 can be use msec timer for user delay timer as TIM2 interrupt service.
This timer used TIM2->CNT get timer2 counter value by "on the fly" and diaplay in main menu 
while 5 sec as end of timer2 (0 - 9999ms), after 5 sec Start 10 kind of graphic display to the LCD screen.
![ili9341-main-menu](https://github.com/user-attachments/assets/8d12af44-7f03-4a3f-9130-59c3272f2666)

High speed LCD graphic demo with SPI-DMA transfer of the main function is intented to examine efect of TIM1 SPWM generation. Delay_Ms timer do not match delay time when using many DMA operation. I was fix it by used TIM2 interrupt for the each different graphic display timer. 

Below screen shots are graphic demo on the TFT LCD ILI9341.
![ILI9341-Random-Dot](https://github.com/user-attachments/assets/00d75460-e9b7-4c21-861d-69b98c488a89)
![ILI9341-Horz-Line](https://github.com/user-attachments/assets/9a079515-36e7-40d4-a967-6a213c061965)
![ILI9341-Vert-Line](https://github.com/user-attachments/assets/131cd7fa-2712-4b42-bd46-e75eaa3f0804)
![ili9341-random-line](https://github.com/user-attachments/assets/6009a794-453f-41b0-a69a-291925bd1789)
![ili9341-center-rect](https://github.com/user-attachments/assets/c55c4a7f-18f6-4b86-b260-e10b2f6e503e)
![ILI9341-random-rect](https://github.com/user-attachments/assets/74fdc83e-6894-4c84-b61c-45b6a1233168)
![ili9341-fill-rect](https://github.com/user-attachments/assets/47aeb06b-14ef-4bad-b8c3-9e1644b9fe00)
![ili9341-move-rectangle](https://github.com/user-attachments/assets/f020c294-46e1-4230-bd6f-a86a06a1c82a)
![ili9341-random-circle](https://github.com/user-attachments/assets/aa8f631c-8608-4c70-99cd-9ec00d9e85f0)
![ili9341-fill-circ](https://github.com/user-attachments/assets/18070c62-80db-4f15-99d5-894aa592cd9e)

Recommanded schematic diagram of WCH-CH32V003F4P6 ILI9341 demo.
![ch32v003f4u6-tssop20-dev-kit-sch](https://github.com/user-attachments/assets/30cffacf-fc64-4348-92b4-b646d2f82b91)

If your have 3.3V device for this project and you will using operating power from USB-C connector, You have to  
buy test PCB included 3.3V LDO. 

Below schematic diagram is USB-to-Serial converter can be use connect vertual COMx port to the PC USB port.
![ch32v003f4p6-usbc-sch](https://github.com/user-attachments/assets/d4c162af-fc2d-487a-8d62-5a66ccd0f641)

All source code made from reference of sample code at CH32V003 EVT for TSSOP-20pin WCH-CH32V003F4P6.
This MPU has advance TIM1, TIM2, ADC, SPI, I2S and DMA functions compatible as ARM STM32F0XX series hardware,
with standard HAL library code style for MounRiver Studio V2.1.0 (used dark theme same as vsCode)
![MounRiver-Studio-V2 10](https://github.com/user-attachments/assets/1a961f86-08be-48a6-9338-c94715416733)

Flash tool used WCH-LinkE-Mini. (GND, SWD, VCC connected to GND, PD1, VCC of CH32V003F4P6).
![ch32v003f4u6-LinkE-Mini-pcb](https://github.com/user-attachments/assets/74a162de-1b7d-4c50-9fc1-7aa25640e6b9)

This tool, almost all WCH microcontrollers (CH5xx, CH6xx, CH32Fxxx, CH32Vxxx, CH32Xxxx, and CH32Lxxx) 
which have a factory-builtin bootloader (v2.x.x) can be flashed via USB. 
And this tool can be support DAP-Link at Keil-UV5 for standard ARM CPU flash to STM32Fxxx seriese.

If you require change mode WCH-LinkE to WCH-LinkDAP (WinUSB or HID), 
WCH-LinkE flash tool can be change WCH-Link mode as press Get and Select mode and Set button in WCH-Link Utilty (V2.40).
If you require restore for return to WCH-LinkE mode, you can use mode set at WCH-LinkE utilty.
