################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/GUI/LVGL_GUI.c \
../Core/Src/GUI/LVGL_GUI_dsp.c \
../Core/Src/GUI/LVGL_GUI_player.c \
../Core/Src/GUI/LVGL_GUI_settings.c \
../Core/Src/GUI/LVGL_GUI_signal_path.c \
../Core/Src/GUI/LVGL_GUI_volume_control.c \
../Core/Src/GUI/LVGL_flush.c \
../Core/Src/GUI/LVGL_input.c \
../Core/Src/GUI/LVGL_toast.c \
../Core/Src/GUI/lv_simple_file_explorer_unicode.c 

OBJS += \
./Core/Src/GUI/LVGL_GUI.o \
./Core/Src/GUI/LVGL_GUI_dsp.o \
./Core/Src/GUI/LVGL_GUI_player.o \
./Core/Src/GUI/LVGL_GUI_settings.o \
./Core/Src/GUI/LVGL_GUI_signal_path.o \
./Core/Src/GUI/LVGL_GUI_volume_control.o \
./Core/Src/GUI/LVGL_flush.o \
./Core/Src/GUI/LVGL_input.o \
./Core/Src/GUI/LVGL_toast.o \
./Core/Src/GUI/lv_simple_file_explorer_unicode.o 

C_DEPS += \
./Core/Src/GUI/LVGL_GUI.d \
./Core/Src/GUI/LVGL_GUI_dsp.d \
./Core/Src/GUI/LVGL_GUI_player.d \
./Core/Src/GUI/LVGL_GUI_settings.d \
./Core/Src/GUI/LVGL_GUI_signal_path.d \
./Core/Src/GUI/LVGL_GUI_volume_control.d \
./Core/Src/GUI/LVGL_flush.d \
./Core/Src/GUI/LVGL_input.d \
./Core/Src/GUI/LVGL_toast.d \
./Core/Src/GUI/lv_simple_file_explorer_unicode.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/GUI/%.o: ../Core/Src/GUI/%.c Core/Src/GUI/subdir.mk
	arm-none-eabi-gcc -fshort-wchar "$<" -mcpu=cortex-m7 -std=gnu11 -DFREERTOS -DPROFILER -DARM_MATH_DSP -D__VFP_FP__ -D__FPU_PRESENT=1 -DUSE_HAL_DRIVER -DSTM32H7B0xx -DARM_MATH_CM7 -c -I../Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_MSC/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/ButtonDebouncer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/RotEnc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/kfifo_DMA" -I../Middlewares/ST/ARM/DSP/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/SD_card" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/Audio" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src/misc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/double_buffer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/PCM1792" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/TLV320ADCx120" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Config" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/ST/ARM/DSP/Lib" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/Target" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/App" -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI/assets" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Profiler" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/encoding" -O2 -ffunction-sections -fdata-sections -Wall -fshort-wchar -Wno-unused-variable -Wno-unused-function -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

