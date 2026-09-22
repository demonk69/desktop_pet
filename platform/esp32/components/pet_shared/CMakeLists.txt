set(PET_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")

idf_component_register(
    SRCS
        "${PET_ROOT}/app/pet/pet_app.c"
        "${PET_ROOT}/animation/pet_animation.c"
        "${PET_ROOT}/animation/pet_builtin_animations.c"
        "${PET_ROOT}/behavior/pet_behavior.c"
        "${PET_ROOT}/config/pet_config.c"
        "${PET_ROOT}/core/pet_core.c"
        "${PET_ROOT}/core/pet_event_queue.c"
        "${PET_ROOT}/hal/backlight.c"
        "${PET_ROOT}/hal/display.c"
        "${PET_ROOT}/services/network/pet_network.c"
        "${PET_ROOT}/services/time/pet_clock_format.c"
        "${PET_ROOT}/services/time/pet_time_service.c"
        "${PET_ROOT}/ui/pet_renderer.c"
        "${PET_ROOT}/ui/pet_scene.c"
    INCLUDE_DIRS "${PET_ROOT}/include"
)
