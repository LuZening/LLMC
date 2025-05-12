################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/lvgl/src/misc/lv_anim.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_anim_timeline.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_area.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_array.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_async.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_bidi.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_color.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_color_op.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_event.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_fs.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_ll.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_log.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_lru.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_math.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_palette.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_profiler_builtin.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_rb.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_style.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_style_gen.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_templ.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_text.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_text_ap.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_timer.c \
../Middlewares/Third_Party/lvgl/src/misc/lv_utils.c 

OBJS += \
./Middlewares/Third_Party/lvgl/src/misc/lv_anim.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_anim_timeline.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_area.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_array.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_async.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_bidi.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_color.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_color_op.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_event.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_fs.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_ll.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_log.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_lru.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_math.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_palette.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_profiler_builtin.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_rb.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_style.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_style_gen.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_templ.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_text.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_text_ap.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_timer.o \
./Middlewares/Third_Party/lvgl/src/misc/lv_utils.o 

C_DEPS += \
./Middlewares/Third_Party/lvgl/src/misc/lv_anim.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_anim_timeline.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_area.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_array.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_async.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_bidi.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_color.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_color_op.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_event.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_fs.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_ll.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_log.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_lru.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_math.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_palette.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_profiler_builtin.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_rb.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_style.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_style_gen.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_templ.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_text.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_text_ap.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_timer.d \
./Middlewares/Third_Party/lvgl/src/misc/lv_utils.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/lvgl/src/misc/%.o: ../Middlewares/Third_Party/lvgl/src/misc/%.c Middlewares/Third_Party/lvgl/src/misc/subdir.mk
	arm-none-eabi-gcc  "$<" -mcpu=cortex-m7 -std=gnu11 -g -DFREERTOS -DTEST_DSP -DDEBUG -DPROFILER -DARM_MATH_DSP -D__VFP_FP__ -D__FPU_PRESENT=1 -DUSE_HAL_DRIVER -DSTM32H7B0xx -DARM_MATH_CM7 -DARM_MATH_MATRIX_CHECK -DARM_MATH_ROUNDING -c -I../Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_MSC/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/ButtonDebouncer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/RotEnc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/kfifo_DMA" -I../Middlewares/ST/ARM/DSP/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/SD_card" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/Audio" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src/misc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/double_buffer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/PCM1792" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/TLV320ADCx120" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Config" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/ST/ARM/DSP/Lib" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/Target" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/App" -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI/assets" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Profiler" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/encoding" -O0 -ffunction-sections -fdata-sections -Wall -fshort-wchar -Wno-unused-variable -Wno-unused-function -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

