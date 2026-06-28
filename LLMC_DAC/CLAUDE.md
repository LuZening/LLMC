# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Editing Rules

- **Only use `Read` and `Edit` tools** for file modifications. Never use `sed`, `awk`, or any shell-based text editing.
- When using `Edit`, always `Read` the file first and copy the exact content (including indentation) from the Read output into `old_string`. Do not manually type or guess indentation characters.

## Build System

Two build systems are supported:

### CMake (primary)
- **Toolchain**: ARM GCC (`arm-none-eabi-gcc`, tested with STM32CubeCLT 14.3)
- **Configure**: `cmake --preset Debug` or `cmake --preset Release`
- **Build**: `cmake --build --preset Debug` or `cmake --build --preset Release`
- **Output**: `build/Debug/LLMC_DAC.elf` / `build/Release/LLMC_DAC.elf`
- **Presets**: `CMakePresets.json` (uses Ninja generator)
- **Toolchain file**: `cmake/gcc-arm-none-eabi.cmake`
- **Component libraries**: `cmake/stm32cubemx/CMakeLists.txt` (HAL, FreeRTOS, FatFs, USB, LVGL, FLAC, Helix MP3 as OBJECT libraries)

### STM32CubeIDE (legacy)
- **IDE**: STM32CubeIDE (`.cproject` project file)
- **Make build**: `make -C Release` or `make -C Debug` from project root
- **Output**: `Release/LLMC_DAC.elf`, `Release/LLMC_DAC.hex`

### Configuration
- **CubeMX config**: `LLMC_DAC.ioc` — regenerate with CubeMX, don't hand-edit
- **Linker script**: `app.ld` (modified for GCC 14 compatibility — VMA addresses for cross-region sections)

## Project Architecture

STM32H7B0-based high-end portable DAC/audio device.

### Memory Map (STM32H7B0, external 16MB QSPI Flash @ 0x90000000)

| Region    | Address     | Size  | Usage                        |
|-----------|-------------|-------|------------------------------|
| ITCMRAM   | 0x00000000  | 64K   | Critical code/data           |
| DTCMRAM1  | 0x20000000  | 64K   | FreeRTOS heap, critical data |
| DTCMRAM2  | 0x20010000  | 64K   | Audio filter states          |
| RAM       | 0x24000000  | 1024K | General purpose              |
| RAM_CD    | 0x30000000  | 128K  | DSP heap                     |
| RAM_SRD   | 0x38000000  | 32K   | Additional SRAM              |

### Top-Level Source Directories

- **Core/Src/** — Application code (main, FreeRTOS, audio pipeline, GUI)
  - **Audio/** — Audio DSP pipeline: EQ (`audio_DSP_EQ.c`), compressor (`audio_DSP_compressor.c`), reverb (`audio_DSP_reverb.c`), resampler (`audio_DSP_resampler.c`), preset management (`audio_DSP_preset.c`), player (`audio_player.c`), audio I/O (`audio_inputs.c`, `audio_outputs.c`), settings (`audio_settings.c`), WAV/MP3 utils
  - **GUI/** — LVGL UI screens (`LVGL_GUI.c`, `LVGL_GUI_settings.c`, `LVGL_GUI_player.c`, `LVGL_GUI_dsp.c`, `LVGL_GUI_signal_path.c`, `LVGL_GUI_volume_control.c`), display driver (`LVGL_flush.c`), input (`LVGL_input.c`), assets, fonts
  - **encoding/** — UTF conversion utilities
- **MyUSBClass/** — Custom USB class drivers (UAC1.0 and Mass Storage Class)
- **MyUSBDevice/** — USB device application layer (descriptors, interface callbacks)
- **Drivers/** — Custom board drivers (PCM179x DAC, TLV320ADCx120 ADC, touchscreen HR2046, rotary encoder, double buffer, kfifo DMA, button debouncer, SD card, profiler)
- **FATFS/** — FatFS integration for SD card storage
- **Middlewares/ST/** — ST USB device library
- **Middlewares/Third_Party/** — FreeRTOS, FatFS, LVGL, Helix MP3 decoder, FLAC decoder

### Audio Data Flow (Ultra-Low-Latency Pipeline)

```
Microphones (I2S) → SAI2 RX DMA → Double Buffer → FIFO → DSP Chain → Mixer → FIFO → SAI2 TX DMA → DAC
USB Playback                     → FIFO → DSP Chain → Mixer → FIFO → USB IN (recording)
SD Card File                     → FIFO → Decoder → DSP Chain
```

- **Sample rate**: 96kHz internal, 32-bit float processing
- **Sample rate conversion**: 44.1K↔48K↔96K via polyphase FIR sinc filters
- **DMA-driven**: I2S DMA double-buffering with half/full transfer callbacks
- **DSP chain per input source**: EQ (8-band biquad IIR) → Compressor → Reverb (comb+allpass)

### Peripherals

| Peripheral | Usage                                        |
|------------|----------------------------------------------|
| SAI2       | I2S audio I/O (Block A=RX encoder, Block B=TX decoder) |
| LTDC       | TFT LCD display controller (480x272)         |
| DMA2D      | Chrom-ART graphics accelerator for LVGL      |
| USB3300    | High-speed USB OTG (UAC1.0 + MSC)            |
| SDMMC1     | SD card for media files + config             |
| OSPI       | External flash (16MB, code + data)           |
| I2C1       | DAC/ADC register configuration               |
| SPI1       | Touchscreen controller (HR2046)              |
| TIM2       | Button debouncing                            |
| TIM5       | SAI clock counter                            |

### FreeRTOS Tasks

- **defaultTask** (2048B stack, BelowNormal) — startup, then idle
- **taskLVGLTimer** (10KB stack, Normal) — LVGL timer handler
- **taskHumanInput** (2KB stack, AboveNormal) — rotary encoder + button input
- **taskFSLoader** — SD card file reading for audio player
- **TouchScreenTask** — touchscreen polling
- **TouchScreenCalibTask** — touchscreen calibration

### Key Configuration Files

- `Core/Inc/Config_user_define.h` — User configuration struct definition (volume, sample rate, DSP presets, USB config, connections)
- `Core/Inc/Audio/audio_settings.h` — Audio sample rate/bit depth enums, default rates
- `Core/Inc/Audio/audio_DSP.h` — DSP types (EQ, compressor, reverb config + state structs)
- `Core/Inc/Audio/audio_buffers.h` — FIFO declarations, input/output routing definitions

### USB Configuration

- Dual personality: UAC1.0 (audio device) or Mass Storage Class
- Switchable at runtime via `USB_switch_to_configuration()`
- Config persistence in `Config.sUsbCfgDescSelector`
