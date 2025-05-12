################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Audio/audio_DSP.c \
../Core/Src/Audio/audio_DSP_EQ.c \
../Core/Src/Audio/audio_DSP_compressor.c \
../Core/Src/Audio/audio_DSP_preset.c \
../Core/Src/Audio/audio_DSP_resampler.c \
../Core/Src/Audio/audio_DSP_reverb.c \
../Core/Src/Audio/audio_DSP_test.c \
../Core/Src/Audio/audio_filter_params.c \
../Core/Src/Audio/audio_inputs.c \
../Core/Src/Audio/audio_outputs.c \
../Core/Src/Audio/audio_player.c \
../Core/Src/Audio/audio_settings.c \
../Core/Src/Audio/mp3utils.c \
../Core/Src/Audio/playlist.c \
../Core/Src/Audio/precise_counter.c \
../Core/Src/Audio/wav.c 

OBJS += \
./Core/Src/Audio/audio_DSP.o \
./Core/Src/Audio/audio_DSP_EQ.o \
./Core/Src/Audio/audio_DSP_compressor.o \
./Core/Src/Audio/audio_DSP_preset.o \
./Core/Src/Audio/audio_DSP_resampler.o \
./Core/Src/Audio/audio_DSP_reverb.o \
./Core/Src/Audio/audio_DSP_test.o \
./Core/Src/Audio/audio_filter_params.o \
./Core/Src/Audio/audio_inputs.o \
./Core/Src/Audio/audio_outputs.o \
./Core/Src/Audio/audio_player.o \
./Core/Src/Audio/audio_settings.o \
./Core/Src/Audio/mp3utils.o \
./Core/Src/Audio/playlist.o \
./Core/Src/Audio/precise_counter.o \
./Core/Src/Audio/wav.o 

C_DEPS += \
./Core/Src/Audio/audio_DSP.d \
./Core/Src/Audio/audio_DSP_EQ.d \
./Core/Src/Audio/audio_DSP_compressor.d \
./Core/Src/Audio/audio_DSP_preset.d \
./Core/Src/Audio/audio_DSP_resampler.d \
./Core/Src/Audio/audio_DSP_reverb.d \
./Core/Src/Audio/audio_DSP_test.d \
./Core/Src/Audio/audio_filter_params.d \
./Core/Src/Audio/audio_inputs.d \
./Core/Src/Audio/audio_outputs.d \
./Core/Src/Audio/audio_player.d \
./Core/Src/Audio/audio_settings.d \
./Core/Src/Audio/mp3utils.d \
./Core/Src/Audio/playlist.d \
./Core/Src/Audio/precise_counter.d \
./Core/Src/Audio/wav.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Audio/%.o: ../Core/Src/Audio/%.c Core/Src/Audio/subdir.mk
	arm-none-eabi-gcc -fshort-wchar "$<" -mcpu=cortex-m7 -std=gnu11 -DFREERTOS -DPROFILER -DARM_MATH_DSP -D__VFP_FP__ -D__FPU_PRESENT=1 -DUSE_HAL_DRIVER -DSTM32H7B0xx -DARM_MATH_CM7 -c -I../Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_MSC/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/ButtonDebouncer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/RotEnc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/kfifo_DMA" -I../Middlewares/ST/ARM/DSP/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/SD_card" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/Audio" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src/misc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/double_buffer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/PCM1792" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/TLV320ADCx120" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Config" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/ST/ARM/DSP/Lib" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/Target" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/App" -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI/assets" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Profiler" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/encoding" -O2 -ffunction-sections -fdata-sections -Wall -fshort-wchar -Wno-unused-variable -Wno-unused-function -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

