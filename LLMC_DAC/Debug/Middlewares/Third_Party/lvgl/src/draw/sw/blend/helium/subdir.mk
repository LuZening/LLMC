################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../Middlewares/Third_Party/lvgl/src/draw/sw/blend/helium/lv_blend_helium.S 

OBJS += \
./Middlewares/Third_Party/lvgl/src/draw/sw/blend/helium/lv_blend_helium.o 

S_UPPER_DEPS += \
./Middlewares/Third_Party/lvgl/src/draw/sw/blend/helium/lv_blend_helium.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/lvgl/src/draw/sw/blend/helium/%.o: ../Middlewares/Third_Party/lvgl/src/draw/sw/blend/helium/%.S Middlewares/Third_Party/lvgl/src/draw/sw/blend/helium/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m7 -g -DDEBUG -c -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/ButtonDebouncer" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_MSC/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/RotEnc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/kfifo_DMA" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/SD_card" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_MSC/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/Audio" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src/misc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/double_buffer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/PCM1792" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/TLV320ADCx120" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Config" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/ST/ARM/DSP/Lib" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/Target" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/App" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/FatFs/src" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI/assets" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Profiler" -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

