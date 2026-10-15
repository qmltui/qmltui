qt_add_executable(qmltui_animated_sprite_demo main.cpp)

qt_add_qml_module(qmltui_animated_sprite_demo
    URI AnimatedSpriteDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        loading.png
    DEPENDENCIES
        TARGET qmltui::core
        TARGET qmltui::image
)

target_link_libraries(qmltui_animated_sprite_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml qmltui::core qmltui::image)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_animated_sprite_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(
        NAME animated_sprite_demo
        TARGET qmltui_animated_sprite_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_animated_sprite_demo.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
        LABELS AgentSmoke
    )
endif()
