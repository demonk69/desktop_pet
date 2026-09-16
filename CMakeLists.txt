cmake_minimum_required(VERSION 3.16)

project(desktop_pet VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PET_BUILD_SIMULATOR "Build the dependency-free PC simulator" ON)
option(PET_BUILD_TESTS "Build unit tests" ON)
option(PET_BUILD_DRIVER_STUBS "Build unverified embedded driver stubs" ON)

add_library(pet_core
    app/pet/pet_app.c
    animation/pet_animation.c
    animation/pet_builtin_animations.c
    core/pet_core.c
    core/pet_event_queue.c
    config/pet_config.c
)
target_include_directories(pet_core PUBLIC include)
target_compile_options(pet_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(pet_ui
    ui/pet_renderer.c
    hal/display.c
    hal/backlight.c
)
target_include_directories(pet_ui PUBLIC include)
target_link_libraries(pet_ui PUBLIC pet_core)
target_compile_options(pet_ui PRIVATE -Wall -Wextra -Wpedantic)

if(PET_BUILD_DRIVER_STUBS)
    add_library(pet_driver_stubs
        drivers/display/st7789_stub.c
        drivers/display/spi_backend_stub.c
        drivers/display/i8080_backend_stub.c
        drivers/backlight/pwm_backlight_stub.c
    )
    target_include_directories(pet_driver_stubs PUBLIC include)
    target_compile_options(pet_driver_stubs PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(PET_BUILD_SIMULATOR)
    add_executable(pet_simulator
        simulator/main.c
        simulator/framebuffer_display.c
    )
    target_include_directories(pet_simulator PRIVATE include simulator)
    target_link_libraries(pet_simulator PRIVATE pet_core pet_ui)
    target_compile_options(pet_simulator PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(PET_BUILD_TESTS)
    enable_testing()

    function(add_pet_test test_name source)
        add_executable(${test_name} ${source})
        target_include_directories(${test_name} PRIVATE include)
        target_link_libraries(${test_name} PRIVATE pet_core)
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    add_pet_test(test_pet_core tests/test_pet_core.c)
    add_pet_test(test_animation tests/test_animation.c)
    add_pet_test(test_event_queue tests/test_event_queue.c)
    add_pet_test(test_config tests/test_config.c)
endif()
