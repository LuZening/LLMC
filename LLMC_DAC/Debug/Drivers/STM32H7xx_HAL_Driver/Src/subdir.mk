################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma2d.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_iwdg.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ospi.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_delayblock.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_sdmmc.c \
../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c 

OBJS += \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma2d.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_iwdg.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ospi.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_delayblock.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_sdmmc.o \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.o 

C_DEPS += \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma2d.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_iwdg.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ospi.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_delayblock.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_sdmmc.d \
./Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/STM32H7xx_HAL_Driver/Src/%.o: ../Drivers/STM32H7xx_HAL_Driver/Src/%.c Drivers/STM32H7xx_HAL_Driver/Src/subdir.mk
	arm-none-eabi-gcc  "$<" -mcpu=cortex-m7 -std=gnu11 -g -DFREERTOS -DTEST_DSP -DDEBUG -DPROFILER -DARM_MATH_DSP -D__VFP_FP__ -D__FPU_PRESENT=1 -DUSE_HAL_DRIVER -DSTM32H7B0xx -DARM_MATH_CM7 -DARM_MATH_MATRIX_CHECK -DARM_MATH_ROUNDING -c -I../Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_MSC/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/ButtonDebouncer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/RotEnc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/kfifo_DMA" -I../Middlewares/ST/ARM/DSP/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/SD_card" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0/Inc" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/Audio" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBClass/MY_UAC1_0" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party/lvgl/src/misc" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/double_buffer" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/PCM1792" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/TLV320ADCx120" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Config" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/Third_Party" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares/ST/ARM/DSP/Lib" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/Target" -I"E:/Projects/You/LLMC/LLMC_DAC/MyUSBDevice/App" -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/GUI/assets" -I"E:/Projects/You/LLMC/LLMC_DAC/Drivers/Profiler" -I"E:/Projects/You/LLMC/LLMC_DAC/Middlewares" -I"E:/Projects/You/LLMC/LLMC_DAC/Core/Inc/encoding" -O0 -ffunction-sections -fdata-sections -Wall -fshort-wchar -Wno-unused-variable -Wno-unused-function -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

