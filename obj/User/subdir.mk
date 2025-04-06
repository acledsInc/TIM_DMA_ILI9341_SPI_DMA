################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/ch32v00x_it.c \
../User/delay.c \
../User/ili9341.c \
../User/main.c \
../User/system_ch32v00x.c \
../User/uart.c 

C_DEPS += \
./User/ch32v00x_it.d \
./User/delay.d \
./User/ili9341.d \
./User/main.d \
./User/system_ch32v00x.d \
./User/uart.d 

OBJS += \
./User/ch32v00x_it.o \
./User/delay.o \
./User/ili9341.o \
./User/main.o \
./User/system_ch32v00x.o \
./User/uart.o 



# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU RISC-V Cross C Compiler'
	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"d:/CH32V003/TIM_DMA-ILI9341-SPI-DMA/Debug" -I"d:/CH32V003/TIM_DMA-ILI9341-SPI-DMA/Core" -I"d:/CH32V003/TIM_DMA-ILI9341-SPI-DMA/User" -I"d:/CH32V003/TIM_DMA-ILI9341-SPI-DMA/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@
