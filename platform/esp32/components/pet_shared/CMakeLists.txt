set(PET_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")

idf_component_register(
    SRCS
        "${PET_ROOT}/app/pet/pet_app.c"
        "${PET_ROOT}/animation/pet_animation.c"
        "${PET_ROOT}/animation/pet_builtin_animations.c"
        "${PET_ROOT}/config/pet_config.c"
        "${PET_ROOT}/core/pet_core.c"
        "${PET_ROOT}/core/pet_event_queue.c"
        "${PET_ROOT}/hal/backlight.c"
        "${PET_ROOT}/hal/display.c"
        "${PET_ROOT}/ui/pet_renderer.c"
    INCLUDE_DIRS "${PET_ROOT}/include"
)
