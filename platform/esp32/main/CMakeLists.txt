idf_component_register(
    SRCS
        "app_main.c"
        "pet_compiled_assets.c"
        "pet_esp32_backlight.c"
        "pet_esp32_board_config.c"
        "pet_esp32_diag.c"
        "pet_esp32_display.c"
        "pet_esp32_runtime.c"
        "pet_esp32_time.c"
    INCLUDE_DIRS "."
    REQUIRES pet_shared
    PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_psram esp_timer heap freertos
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    PET_LCD_SPI_FREQUENCY_HZ=${PET_LCD_SPI_FREQUENCY_HZ}
    PET_LCD_STAGING_BUFFER_SIZE=${PET_LCD_STAGING_BUFFER_SIZE}
    PET_LCD_DMA_ENABLED=${PET_LCD_DMA_ENABLED}
    PET_LCD_BLOCK_HEIGHT=${PET_LCD_BLOCK_HEIGHT}
    PET_LCD_DIAG_MODE=${PET_LCD_DIAG_MODE}
    PET_LCD_DIAG_TRANSPORT=${PET_LCD_DIAG_TRANSPORT}
    PET_ESP32_TARGET_FRAME_US=${PET_ESP32_TARGET_FRAME_US}
)
