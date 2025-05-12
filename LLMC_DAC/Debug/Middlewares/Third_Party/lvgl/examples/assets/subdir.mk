################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/lvgl/examples/assets/animimg001.c \
../Middlewares/Third_Party/lvgl/examples/assets/animimg002.c \
../Middlewares/Third_Party/lvgl/examples/assets/animimg003.c \
../Middlewares/Third_Party/lvgl/examples/assets/img_caret_down.c \
../Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_argb.c \
../Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_indexed16.c \
../Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_rgb.c \
../Middlewares/Third_Party/lvgl/examples/assets/img_hand.c \
../Middlewares/Third_Party/lvgl/examples/assets/img_skew_strip.c \
../Middlewares/Third_Party/lvgl/examples/assets/img_star.c \
../Middlewares/Third_Party/lvgl/examples/assets/imgbtn_left.c \
../Middlewares/Third_Party/lvgl/examples/assets/imgbtn_mid.c \
../Middlewares/Third_Party/lvgl/examples/assets/imgbtn_right.c 

OBJS += \
./Middlewares/Third_Party/lvgl/examples/assets/animimg001.o \
./Middlewares/Third_Party/lvgl/examples/assets/animimg002.o \
./Middlewares/Third_Party/lvgl/examples/assets/animimg003.o \
./Middlewares/Third_Party/lvgl/examples/assets/img_caret_down.o \
./Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_argb.o \
./Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_indexed16.o \
./Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_rgb.o \
./Middlewares/Third_Party/lvgl/examples/assets/img_hand.o \
./Middlewares/Third_Party/lvgl/examples/assets/img_skew_strip.o \
./Middlewares/Third_Party/lvgl/examples/assets/img_star.o \
./Middlewares/Third_Party/lvgl/examples/assets/imgbtn_left.o \
./Middlewares/Third_Party/lvgl/examples/assets/imgbtn_mid.o \
./Middlewares/Third_Party/lvgl/examples/assets/imgbtn_right.o 

C_DEPS += \
./Middlewares/Third_Party/lvgl/examples/assets/animimg001.d \
./Middlewares/Third_Party/lvgl/examples/assets/animimg002.d \
./Middlewares/Third_Party/lvgl/examples/assets/animimg003.d \
./Middlewares/Third_Party/lvgl/examples/assets/img_caret_down.d \
./Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_argb.d \
./Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_indexed16.d \
./Middlewares/Third_Party/lvgl/examples/assets/img_cogwheel_rgb.d \
./Middlewares/Third_Party/lvgl/examples/assets/img_hand.d \
./Middlewares/Third_Party/lvgl/examples/assets/img_skew_strip.d \
./Middlewares/Third_Party/lvgl/examples/assets/img_star.d \
./Middlewares/Third_Party/lvgl/examples/assets/imgbtn_left.d \
./Middlewares/Third_Party/lvgl/examples/assets/imgbtn_mid.d \
./Middlewares/Third_Party/lvgl/examples/assets/imgbtn_right.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/lvgl/examples/assets/%.o: ../Middlewares/Third_Party/lvgl/examples/assets/%.c Middlewares/Third_Party/lvgl/examples/assets/subdir.mk
	arm-none-eabi-gcc  "$<" -mcpu=cortex-m7 -std=gnu11 -g -DFREERTOS -DTEST_DSP -DDEBUG -DPROFILER -DARM_MATH_DSP -D__VFP_FP__ -D__FPU_PRESENT=1 -DUSE_HAL_DRIVER -DSTM32H7B0xx -DARM_MATH_CM7 -DARM_MATH_MATRIX_CHECK -DARM_MATH_ROUNDING -c -I../Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_MSC/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/ButtonDebouncer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/RotEnc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/kfifo_DMA" -I../Middlewares/ST/ARM/DSP/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/SD_card" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/Audio" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src/misc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/double_buffer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/PCM1792" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/TLV320ADCx120" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Config" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/ST/ARM/DSP/Lib" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/Target" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/App" -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI/assets" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Profiler" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/encoding" -O0 -ffunction-sections -fdata-sections -Wall -fshort-wchar -Wno-unused-variable -Wno-unused-function -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

