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
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(desktop_pet_esp32 VERSION 0.4.0)
