################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/GUI/assets/LLMC_LVGL_assets.c \
../Core/Src/GUI/assets/back_img_player_1.c \
../Core/Src/GUI/assets/back_img_settings_1.c \
../Core/Src/GUI/assets/btn_img_1.c \
../Core/Src/GUI/assets/btn_img_2.c \
../Core/Src/GUI/assets/btn_img_3.c \
../Core/Src/GUI/assets/character_icon.c \
../Core/Src/GUI/assets/character_icon_small_transp.c \
../Core/Src/GUI/assets/farewell_img.c \
../Core/Src/GUI/assets/img_btn_loop.c \
../Core/Src/GUI/assets/img_btn_next.c \
../Core/Src/GUI/assets/img_btn_pause.c \
../Core/Src/GUI/assets/img_btn_play.c \
../Core/Src/GUI/assets/img_btn_prev.c \
../Core/Src/GUI/assets/img_btn_rnd.c \
../Core/Src/GUI/assets/img_slider_knob.c \
../Core/Src/GUI/assets/main_background_1.c \
../Core/Src/GUI/assets/main_background_2.c \
../Core/Src/GUI/assets/main_background_3.c \
../Core/Src/GUI/assets/main_background_4.c \
../Core/Src/GUI/assets/school_logo.c \
../Core/Src/GUI/assets/school_logo_inactive.c \
../Core/Src/GUI/assets/warning_img.c 

OBJS += \
./Core/Src/GUI/assets/LLMC_LVGL_assets.o \
./Core/Src/GUI/assets/back_img_player_1.o \
./Core/Src/GUI/assets/back_img_settings_1.o \
./Core/Src/GUI/assets/btn_img_1.o \
./Core/Src/GUI/assets/btn_img_2.o \
./Core/Src/GUI/assets/btn_img_3.o \
./Core/Src/GUI/assets/character_icon.o \
./Core/Src/GUI/assets/character_icon_small_transp.o \
./Core/Src/GUI/assets/farewell_img.o \
./Core/Src/GUI/assets/img_btn_loop.o \
./Core/Src/GUI/assets/img_btn_next.o \
./Core/Src/GUI/assets/img_btn_pause.o \
./Core/Src/GUI/assets/img_btn_play.o \
./Core/Src/GUI/assets/img_btn_prev.o \
./Core/Src/GUI/assets/img_btn_rnd.o \
./Core/Src/GUI/assets/img_slider_knob.o \
./Core/Src/GUI/assets/main_background_1.o \
./Core/Src/GUI/assets/main_background_2.o \
./Core/Src/GUI/assets/main_background_3.o \
./Core/Src/GUI/assets/main_background_4.o \
./Core/Src/GUI/assets/school_logo.o \
./Core/Src/GUI/assets/school_logo_inactive.o \
./Core/Src/GUI/assets/warning_img.o 

C_DEPS += \
./Core/Src/GUI/assets/LLMC_LVGL_assets.d \
./Core/Src/GUI/assets/back_img_player_1.d \
./Core/Src/GUI/assets/back_img_settings_1.d \
./Core/Src/GUI/assets/btn_img_1.d \
./Core/Src/GUI/assets/btn_img_2.d \
./Core/Src/GUI/assets/btn_img_3.d \
./Core/Src/GUI/assets/character_icon.d \
./Core/Src/GUI/assets/character_icon_small_transp.d \
./Core/Src/GUI/assets/farewell_img.d \
./Core/Src/GUI/assets/img_btn_loop.d \
./Core/Src/GUI/assets/img_btn_next.d \
./Core/Src/GUI/assets/img_btn_pause.d \
./Core/Src/GUI/assets/img_btn_play.d \
./Core/Src/GUI/assets/img_btn_prev.d \
./Core/Src/GUI/assets/img_btn_rnd.d \
./Core/Src/GUI/assets/img_slider_knob.d \
./Core/Src/GUI/assets/main_background_1.d \
./Core/Src/GUI/assets/main_background_2.d \
./Core/Src/GUI/assets/main_background_3.d \
./Core/Src/GUI/assets/main_background_4.d \
./Core/Src/GUI/assets/school_logo.d \
./Core/Src/GUI/assets/school_logo_inactive.d \
./Core/Src/GUI/assets/warning_img.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/GUI/assets/%.o: ../Core/Src/GUI/assets/%.c Core/Src/GUI/assets/subdir.mk
	arm-none-eabi-gcc  "$<" -mcpu=cortex-m7 -std=gnu11 -g -DFREERTOS -DTEST_DSP -DDEBUG -DPROFILER -DARM_MATH_DSP -D__VFP_FP__ -D__FPU_PRESENT=1 -DUSE_HAL_DRIVER -DSTM32H7B0xx -DARM_MATH_CM7 -DARM_MATH_MATRIX_CHECK -DARM_MATH_ROUNDING -c -I../Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_MSC/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/ButtonDebouncer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/RotEnc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/kfifo_DMA" -I../Middlewares/ST/ARM/DSP/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/SD_card" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/Audio" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src/misc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/double_buffer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/PCM1792" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/TLV320ADCx120" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Config" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/ST/ARM/DSP/Lib" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/Target" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/App" -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI/assets" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Profiler" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/encoding" -O0 -ffunction-sections -fdata-sections -Wall -fshort-wchar -Wno-unused-variable -Wno-unused-function -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

