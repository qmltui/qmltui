qt_add_executable(qamui_animated_sprite_demo main.cpp)

qt_add_qml_module(qamui_animated_sprite_demo
    URI AnimatedSpriteDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        loading.png
    DEPENDENCIES TARGET
        Qamui::Core
        Qamui::Image
)

target_link_libraries(qamui_animated_sprite_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qamui::Core Qamui::Image)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_animated_sprite_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(
        NAME animated_sprite_demo
        TARGET qamui_animated_sprite_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_animated_sprite_demo.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
        LABELS AgentSmoke
    )
endif()
