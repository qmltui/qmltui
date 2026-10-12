qt_add_executable(qmltui_image_demo main.cpp)

qt_add_qml_module(qmltui_image_demo
    URI ImageDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        demo_image.png
        panda.png
        map.png

    DEPENDENCIES TARGET
        qmltui::core
        qmltui::controls
        qmltui::image
)

target_link_libraries(qmltui_image_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml qmltui::core qmltui::controls qmltui::image)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_image_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(
        NAME image_demo_render_modes
        TARGET qmltui_image_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_image_demo.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
