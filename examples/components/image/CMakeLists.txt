qt_add_executable(qamui_image_demo main.cpp)

qt_add_qml_module(qamui_image_demo
    URI ImageDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        demo_image.png
        panda.png
        map.png

    DEPENDENCIES TARGET
        Qamui::Core
        Qamui::Controls
        Qamui::Image
)

target_link_libraries(qamui_image_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qamui::Core Qamui::Controls Qamui::Image)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_image_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(
        NAME image_demo_render_modes
        TARGET qamui_image_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_image_demo.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
