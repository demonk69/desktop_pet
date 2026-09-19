cmake_minimum_required(VERSION 3.22)

set(IDF_TARGET "esp32s3")
set(PET_LCD_SPI_FREQUENCY_HZ "40000000" CACHE STRING "LCD SPI clock in Hz")
set(PET_LCD_STAGING_BUFFER_SIZE "4096" CACHE STRING "LCD internal staging bytes")
set(PET_LCD_DMA_ENABLED "1" CACHE STRING "Enable LCD SPI DMA")
set(PET_LCD_BLOCK_HEIGHT "0" CACHE STRING "LCD transfer block height; 0 uses staging size")
set(PET_LCD_DIAG_MODE "NONE" CACHE STRING
    "LCD diagnostic mode: NONE, STATIC_ONCE, STATIC_REPEAT")
set(PET_LCD_DIAG_TRANSPORT "DEFAULT" CACHE STRING
    "LCD diagnostic transport: DEFAULT, V04, V05, ROW8")
set(PET_ESP32_TARGET_FRAME_US "100000" CACHE STRING
    "ESP32 runtime target frame period in microseconds")
set(PET_ROTARY_GA_GPIO "4" CACHE STRING "Rotary GA/SIGA GPIO (verified)")
set(PET_ROTARY_BB_GPIO "5" CACHE STRING "Rotary BB/SIGB GPIO (verified)")
set(PET_ROTARY_GA_PULL "NONE" CACHE STRING
    "Rotary GA pull mode: NONE, UP, DOWN (module has external pull-ups)")
set(PET_ROTARY_BB_PULL "NONE" CACHE STRING
    "Rotary BB pull mode: NONE, UP, DOWN (module has external pull-ups)")
set(PET_ROTARY_TRANSITIONS_PER_DETENT "4" CACHE STRING
    "Rotary quadrature transitions per detent (verified)")
set(PET_ROTARY_ADC_LOW_MAX_MV "600" CACHE STRING
    "Rotary GA ADC LOW class upper bound in mV (encoder A contact)")
set(PET_ROTARY_ADC_PRESS_MIN_MV "1000" CACHE STRING
    "Rotary GA ADC PRESS class lower bound in mV")
set(PET_ROTARY_ADC_PRESS_MAX_MV "2200" CACHE STRING
    "Rotary GA ADC PRESS class upper bound in mV")
set(PET_ROTARY_ADC_HIGH_MIN_MV "2600" CACHE STRING
    "Rotary GA ADC HIGH class lower bound in mV (idle)")
set(PET_ROTARY_PRESS_DEBOUNCE_MS "15" CACHE STRING
    "Rotary press/release debounce in ms")
set(PET_ROTARY_DIAG_MODE "NONE" CACHE STRING
    "Rotary diagnostic mode: NONE, ROTARY_HW_DIAG, ADC_MARGIN")
set(PET_ROTARY_HW_DIAG_GA_GPIO "4" CACHE STRING
    "Rotary hardware diagnostic GA/SIGA GPIO")
set(PET_ROTARY_HW_DIAG_BB_GPIO "5" CACHE STRING
    "Rotary hardware diagnostic BB/SIGB GPIO")
set(PET_BACKLIGHT_PWM_FREQ_HZ "20000" CACHE STRING
    "[HW_VERIFY] Backlight LEDC PWM frequency in Hz")
set(PET_BACKLIGHT_PWM_RESOLUTION "10" CACHE STRING
    "[HW_VERIFY] Backlight LEDC PWM resolution bits")
set(PET_BACKLIGHT_DEFAULT_PERCENT "100" CACHE STRING
    "Backlight brightness after the first rendered frame")
set(PET_BACKLIGHT_SLEEP_PERCENT "0" CACHE STRING
    "Backlight brightness while the pet is sleeping")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(desktop_pet_esp32 VERSION 0.4.0)
