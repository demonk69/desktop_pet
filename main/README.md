# Embedded Entry Point

The verified embedded target is ESP32-S3 with ESP-IDF 6.1. Its real entry point and
HAL backends live in `platform/esp32/`; this directory remains a platform-neutral
placeholder for any future embedded target rather than duplicating `app_main`.
