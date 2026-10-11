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

if(TARGET Qamui::Runner AND QAMUI_BUILD_TESTS)
    qt_add_executable(qamui_animated_sprite_demo_test main.cpp)
    qt_add_qml_module(qamui_animated_sprite_demo_test
        URI AnimatedSpriteDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/AnimatedSpriteDemo"
        VERSION 1.0
        QML_FILES Main.qml
        RESOURCES
            loading.png
        DEPENDENCIES TARGET
            Qamui::Core
            Qamui::Image
    )
    target_link_libraries(qamui_animated_sprite_demo_test PRIVATE
        Qt6::Core Qt6::Gui Qt6::Qml Qamui::Core Qamui::Image Qamui::Runner)
    qamui_add_scenario_test(
        NAME animated_sprite_demo
        TARGET qamui_animated_sprite_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_animated_sprite_demo.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
        LABELS AgentSmoke
    )
endif()
