################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../GameBoy/Src/gameboy_bridge.cpp 

CC_SRCS += \
../GameBoy/Src/gb.cc \
../GameBoy/Src/gb_step.cc \
../GameBoy/Src/interrupts.cc \
../GameBoy/Src/loop.cc \
../GameBoy/Src/mbc.cc \
../GameBoy/Src/mmu.cc \
../GameBoy/Src/opcodes.cc \
../GameBoy/Src/ppu.cc \
../GameBoy/Src/ttd.cc 

C_SRCS += \
../GameBoy/Src/gameboy_display.c \
../GameBoy/Src/gameboy_rom.c \
../GameBoy/Src/gd_usb.c 

C_DEPS += \
./GameBoy/Src/gameboy_display.d \
./GameBoy/Src/gameboy_rom.d \
./GameBoy/Src/gd_usb.d 

CC_DEPS += \
./GameBoy/Src/gb.d \
./GameBoy/Src/gb_step.d \
./GameBoy/Src/interrupts.d \
./GameBoy/Src/loop.d \
./GameBoy/Src/mbc.d \
./GameBoy/Src/mmu.d \
./GameBoy/Src/opcodes.d \
./GameBoy/Src/ppu.d \
./GameBoy/Src/ttd.d 

OBJS += \
./GameBoy/Src/gameboy_bridge.o \
./GameBoy/Src/gameboy_display.o \
./GameBoy/Src/gameboy_rom.o \
./GameBoy/Src/gb.o \
./GameBoy/Src/gb_step.o \
./GameBoy/Src/gd_usb.o \
./GameBoy/Src/interrupts.o \
./GameBoy/Src/loop.o \
./GameBoy/Src/mbc.o \
./GameBoy/Src/mmu.o \
./GameBoy/Src/opcodes.o \
./GameBoy/Src/ppu.o \
./GameBoy/Src/ttd.o 

CPP_DEPS += \
./GameBoy/Src/gameboy_bridge.d 


# Each subdirectory must supply rules for building sources it contributes
GameBoy/Src/%.o GameBoy/Src/%.su GameBoy/Src/%.cyclo: ../GameBoy/Src/%.cpp GameBoy/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"C:/Users/Manansh Pandey/STM32CubeIDE/workspace_1.19.0/GameBoy_On_STM32/GameBoy/Inc" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
GameBoy/Src/%.o GameBoy/Src/%.su GameBoy/Src/%.cyclo: ../GameBoy/Src/%.c GameBoy/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"C:/Users/Manansh Pandey/STM32CubeIDE/workspace_1.19.0/GameBoy_On_STM32/GameBoy/Inc" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
GameBoy/Src/%.o GameBoy/Src/%.su GameBoy/Src/%.cyclo: ../GameBoy/Src/%.cc GameBoy/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"C:/Users/Manansh Pandey/STM32CubeIDE/workspace_1.19.0/GameBoy_On_STM32/GameBoy/Inc" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-GameBoy-2f-Src

clean-GameBoy-2f-Src:
	-$(RM) ./GameBoy/Src/gameboy_bridge.cyclo ./GameBoy/Src/gameboy_bridge.d ./GameBoy/Src/gameboy_bridge.o ./GameBoy/Src/gameboy_bridge.su ./GameBoy/Src/gameboy_display.cyclo ./GameBoy/Src/gameboy_display.d ./GameBoy/Src/gameboy_display.o ./GameBoy/Src/gameboy_display.su ./GameBoy/Src/gameboy_rom.cyclo ./GameBoy/Src/gameboy_rom.d ./GameBoy/Src/gameboy_rom.o ./GameBoy/Src/gameboy_rom.su ./GameBoy/Src/gb.cyclo ./GameBoy/Src/gb.d ./GameBoy/Src/gb.o ./GameBoy/Src/gb.su ./GameBoy/Src/gb_step.cyclo ./GameBoy/Src/gb_step.d ./GameBoy/Src/gb_step.o ./GameBoy/Src/gb_step.su ./GameBoy/Src/gd_usb.cyclo ./GameBoy/Src/gd_usb.d ./GameBoy/Src/gd_usb.o ./GameBoy/Src/gd_usb.su ./GameBoy/Src/interrupts.cyclo ./GameBoy/Src/interrupts.d ./GameBoy/Src/interrupts.o ./GameBoy/Src/interrupts.su ./GameBoy/Src/loop.cyclo ./GameBoy/Src/loop.d ./GameBoy/Src/loop.o ./GameBoy/Src/loop.su ./GameBoy/Src/mbc.cyclo ./GameBoy/Src/mbc.d ./GameBoy/Src/mbc.o ./GameBoy/Src/mbc.su ./GameBoy/Src/mmu.cyclo ./GameBoy/Src/mmu.d ./GameBoy/Src/mmu.o ./GameBoy/Src/mmu.su ./GameBoy/Src/opcodes.cyclo ./GameBoy/Src/opcodes.d ./GameBoy/Src/opcodes.o ./GameBoy/Src/opcodes.su ./GameBoy/Src/ppu.cyclo ./GameBoy/Src/ppu.d ./GameBoy/Src/ppu.o ./GameBoy/Src/ppu.su ./GameBoy/Src/ttd.cyclo ./GameBoy/Src/ttd.d ./GameBoy/Src/ttd.o ./GameBoy/Src/ttd.su

.PHONY: clean-GameBoy-2f-Src

