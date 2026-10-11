qt_add_executable(qtui_animated_sprite_demo main.cpp)

qt_add_qml_module(qtui_animated_sprite_demo
    URI AnimatedSpriteDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        loading.png
    DEPENDENCIES TARGET
        Qtui::Core
        Qtui::Image
)

target_link_libraries(qtui_animated_sprite_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Image)

if(TARGET Qtui::Runner AND QTUI_BUILD_TESTS)
    qt_add_executable(qtui_animated_sprite_demo_test main.cpp)
    qt_add_qml_module(qtui_animated_sprite_demo_test
        URI AnimatedSpriteDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/AnimatedSpriteDemo"
        VERSION 1.0
        QML_FILES Main.qml
        RESOURCES
            loading.png
        DEPENDENCIES TARGET
            Qtui::Core
            Qtui::Image
    )
    target_link_libraries(qtui_animated_sprite_demo_test PRIVATE
        Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Image Qtui::Runner)
    qtui_add_scenario_test(
        NAME animated_sprite_demo
        TARGET qtui_animated_sprite_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_animated_sprite_demo.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
        LABELS AgentSmoke
    )
endif()
