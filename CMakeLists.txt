cmake_minimum_required(VERSION 3.16)

project(desktop_pet VERSION 0.4.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PET_BUILD_SIMULATOR "Build the dependency-free PC simulator" ON)
option(PET_BUILD_SDL_SIMULATOR "Build the interactive SDL2 simulator when SDL2 is available" ON)
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
    add_library(pet_pc_support
        simulator/framebuffer_display.c
        simulator/file_asset_provider.c
    )
    target_include_directories(pet_pc_support PUBLIC include simulator)
    target_link_libraries(pet_pc_support PUBLIC pet_ui)
    target_compile_options(pet_pc_support PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(pet_simulator
        simulator/main.c
    )
    target_include_directories(pet_simulator PRIVATE include simulator)
    target_link_libraries(pet_simulator PRIVATE pet_core pet_ui pet_pc_support)
    target_compile_options(pet_simulator PRIVATE -Wall -Wextra -Wpedantic)

    if(PET_BUILD_SDL_SIMULATOR)
        find_package(SDL2 2.0.18 QUIET CONFIG)
        if(TARGET SDL2::SDL2)
            set(PET_SDL2_TARGET SDL2::SDL2)
        else()
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2>=2.0.18)
                if(TARGET PkgConfig::SDL2)
                    set(PET_SDL2_TARGET PkgConfig::SDL2)
                endif()
            endif()
        endif()

        if(PET_SDL2_TARGET)
            add_executable(desktop_pet_simulator
                simulator/sdl_main.c
                simulator/sdl_display.c
                simulator/simulator_input.c
            )
            target_include_directories(desktop_pet_simulator PRIVATE include simulator)
            target_link_libraries(desktop_pet_simulator PRIVATE
                pet_core pet_ui pet_pc_support ${PET_SDL2_TARGET})
            target_compile_definitions(desktop_pet_simulator PRIVATE
                PET_DEFAULT_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets/pet")
            target_compile_options(desktop_pet_simulator PRIVATE -Wall -Wextra -Wpedantic)
            message(STATUS "SDL2 found: building desktop_pet_simulator")
        else()
            message(STATUS "SDL2 not found: desktop_pet_simulator will not be built")
        endif()
    endif()
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
    add_pet_test(test_app_integration tests/test_app_integration.c)

    if(PET_BUILD_SIMULATOR)
        add_executable(test_renderer_integration tests/test_renderer_integration.c)
        target_include_directories(test_renderer_integration PRIVATE include simulator)
        target_link_libraries(test_renderer_integration PRIVATE
            pet_core pet_ui pet_pc_support)
        target_compile_definitions(test_renderer_integration PRIVATE
            PET_TEST_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets/pet")
        target_compile_options(test_renderer_integration PRIVATE -Wall -Wextra -Wpedantic)
        add_test(NAME test_renderer_integration COMMAND test_renderer_integration)
    endif()

    if(TARGET desktop_pet_simulator)
        add_executable(test_simulator_input
            tests/test_simulator_input.c
            simulator/simulator_input.c
        )
        target_include_directories(test_simulator_input PRIVATE include simulator)
        target_link_libraries(test_simulator_input PRIVATE ${PET_SDL2_TARGET})
        target_compile_options(test_simulator_input PRIVATE -Wall -Wextra -Wpedantic)
        add_test(NAME test_simulator_input COMMAND test_simulator_input)
    endif()
endif()
