idf_build_get_property(selected_app PASSPORT_APP)
if(selected_app STREQUAL "tuner")
    idf_component_register(
        SRCS "../apps/tuner/main.c" "../apps/tuner/tuner_pitch.c" "../apps/tuner/metronome.c"
        INCLUDE_DIRS "../apps/tuner"
        REQUIRES bsp esp_timer)
else()
    set(card_image "../assets/images/personal-card/card_template.c")
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../assets/private/billy/billy_card.c")
        set(card_image "../assets/private/billy/billy_card.c")
        message(STATUS "Personal card: using ignored local artwork")
    else()
        message(STATUS "Personal card: using redistributable generic template")
    endif()
    idf_component_register(
        SRCS "../apps/personal-card/main.c" "${card_image}"
        INCLUDE_DIRS "."
        REQUIRES bsp esp_timer)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
endif()
