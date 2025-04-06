################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include User/subdir.mk
-include Startup/subdir.mk
-include Peripheral/src/subdir.mk
-include Debug/subdir.mk
-include Core/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
TIM_DMA_ILI9341_SPI_DMA.hex \

SECONDARY_LIST += \
TIM_DMA_ILI9341_SPI_DMA.lst \

SECONDARY_SIZE += \
TIM_DMA_ILI9341_SPI_DMA.siz \


# All Target
all: 
	$(MAKE) --no-print-directory main-build 

main-build: TIM_DMA_ILI9341_SPI_DMA.elf secondary-outputs

# Tool invocations
TIM_DMA_ILI9341_SPI_DMA.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU RISC-V Cross C Linker'
	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T "d:/CH32V003/TIM_DMA-ILI9341-SPI-DMA/Ld/Link.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,"TIM_DMA_ILI9341_SPI_DMA.map" --specs=nano.specs --specs=nosys.specs -o "TIM_DMA_ILI9341_SPI_DMA.elf" $(OBJS) $(USER_OBJS) $(LIBS) -lprintf
	@echo 'Finished building target: $@'
	@
TIM_DMA_ILI9341_SPI_DMA.hex: TIM_DMA_ILI9341_SPI_DMA.elf
	@echo 'Invoking: GNU RISC-V Cross Create Flash Image'
	@	riscv-none-embed-objcopy -O ihex "TIM_DMA_ILI9341_SPI_DMA.elf" "TIM_DMA_ILI9341_SPI_DMA.hex"
	@echo 'Finished building $@'
	@
TIM_DMA_ILI9341_SPI_DMA.lst: TIM_DMA_ILI9341_SPI_DMA.elf
	@echo 'Invoking: GNU RISC-V Cross Create Listing'
	@	riscv-none-embed-objdump --all-headers --demangle --disassemble -M xw "TIM_DMA_ILI9341_SPI_DMA.elf" > "TIM_DMA_ILI9341_SPI_DMA.lst"
	@echo 'Finished building $@'
	@
TIM_DMA_ILI9341_SPI_DMA.siz: TIM_DMA_ILI9341_SPI_DMA.elf
	@echo 'Invoking: GNU RISC-V Cross Print Size'
	riscv-none-embed-size --format=berkeley "TIM_DMA_ILI9341_SPI_DMA.elf"
	@echo 'Finished building $@'
	@

# Other Targets
clean:
	-$(RM) $(OBJS)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(S_DEPS)$(S_UPPER_DEPS)$(ASM_DEPS)$(ASM_UPPER_DEPS)$(C_DEPS) TIM_DMA_ILI9341_SPI_DMA.elf

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets