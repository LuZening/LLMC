cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME LLMC_DAC)

# Enable compile command to ease indexing with clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources (HAL, FreeRTOS, FatFs, USB, LVGL, etc.)
add_subdirectory(cmake/stm32cubemx)

# ============================================================
# Startup assembly
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Startup/startup_stm32h7b0vbtx.s
)

# ============================================================
# CubeMX-generated application sources
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/main.c
    Core/Src/freertos.c
    Core/Src/stm32h7xx_it.c
    Core/Src/stm32h7xx_hal_msp.c
    Core/Src/stm32h7xx_hal_timebase_tim.c
    Core/Src/sysmem.c
    Core/Src/syscalls.c
    Core/Src/touch_HR2046.c
    Core/Src/Config_user_define.c

    FATFS/App/fatfs.c
    FATFS/Target/bsp_driver_sd.c
    FATFS/Target/sd_diskio.c

    MyUSBDevice/App/usb_device.c
    MyUSBDevice/App/usbd_desc.c
    MyUSBDevice/App/usbd_audio_if.c
    MyUSBDevice/App/usbd_storage_if.c
    MyUSBDevice/Target/usbd_conf.c
)

# ============================================================
# Custom Drivers
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Drivers/ButtonDebouncer/ButtonDebouncer.c
    Drivers/Config/Config.c
    Drivers/PCM1792/PCM179x.c
    Drivers/PCM1792/PCM179x_I2C.c
    Drivers/Profiler/profiler.c
    Drivers/RotEnc/RotEnc.c
    Drivers/SD_card/SD_card.c
    Drivers/TLV320ADCx120/TLV320ADCx120.c
    Drivers/double_buffer/double_buffer.c
    Drivers/kfifo_DMA/kfifo_DMA.c
)

# ============================================================
# MyUSBClass
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    MyUSBClass/MyUSB.c
    MyUSBClass/MY_UAC1_0/Src/my_usbd_audio.c
    MyUSBClass/MY_MSC/Src/usbd_msc.c
    MyUSBClass/MY_MSC/Src/usbd_msc_bot.c
    MyUSBClass/MY_MSC/Src/usbd_msc_data.c
    MyUSBClass/MY_MSC/Src/usbd_msc_scsi.c
)

# ============================================================
# Audio DSP Pipeline
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/Audio/audio_DSP.c
    Core/Src/Audio/audio_DSP_EQ.c
    Core/Src/Audio/audio_DSP_compressor.c
    Core/Src/Audio/audio_DSP_preset.c
    Core/Src/Audio/audio_DSP_resampler.c
    Core/Src/Audio/audio_DSP_reverb.c
    Core/Src/Audio/audio_DSP_test.c
    Core/Src/Audio/audio_filter_params.c
    Core/Src/Audio/audio_inputs.c
    Core/Src/Audio/audio_outputs.c
    Core/Src/Audio/audio_player.c
    Core/Src/Audio/audio_settings.c
    Core/Src/Audio/decoder_pool.c
Core/Src/Audio/decoder_wav_dr.c
    Core/Src/Audio/decoder_flac.c
    Core/Src/Audio/decoder_mp3.c
    Core/Src/Audio/playlist.c
    Core/Src/Audio/precise_counter.c
    Core/Src/Audio/wav.c
)

# ============================================================
# LVGL GUI
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/GUI/LVGL_GUI.c
    Core/Src/GUI/LVGL_GUI_dsp.c
    Core/Src/GUI/LVGL_GUI_player.c
    Core/Src/GUI/LVGL_GUI_settings.c
    Core/Src/GUI/LVGL_GUI_signal_path.c
    Core/Src/GUI/LVGL_GUI_volume_control.c
    Core/Src/GUI/LVGL_flush.c
    Core/Src/GUI/LVGL_input.c
    Core/Src/GUI/LVGL_toast.c
    Core/Src/GUI/lv_simple_file_explorer_unicode.c
)

# ============================================================
# GUI Assets & Fonts
# ============================================================
file(GLOB_RECURSE GUI_Asset_Src
    "Core/Src/GUI/assets/*.c"
    "Core/Src/GUI/fonts/*.c"
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${GUI_Asset_Src})

# ============================================================
# UTF Encoding utils
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/encoding/utf_convert.c
)

# ============================================================
# Simulator support (PC-side testing)
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/sim.c
)

# Add third-party library include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/dr_libs
)

# Add DSP library search path
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/ARM/DSP/Lib
)

# Suppress warnings in project code
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wno-unused-variable
    -Wno-unused-function
)

# Generate HEX file after build
get_filename_component(ARM_TOOLCHAIN_BIN ${CMAKE_C_COMPILER} DIRECTORY)
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${ARM_TOOLCHAIN_BIN}/arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/${CMAKE_PROJECT_NAME}.hex
    COMMAND ${ARM_TOOLCHAIN_BIN}/arm-none-eabi-size $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Generating HEX file..."
)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    STM32_Drivers
    USB_Device_Library
    FreeRTOS
    FatFs
    LVGL
    arm_cortexM7lfdp_math
    m
)
