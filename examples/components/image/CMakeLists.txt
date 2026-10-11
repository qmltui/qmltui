qt_add_executable(qtui_image_demo main.cpp)

qt_add_qml_module(qtui_image_demo
    URI ImageDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        demo_image.png
        panda.png
        map.png
        
    DEPENDENCIES TARGET
        Qtui::Core
        Qtui::Controls
        Qtui::Image
)

target_link_libraries(qtui_image_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Controls Qtui::Image)

if(TARGET Qtui::Runner AND QTUI_BUILD_TESTS)
    qt_add_executable(qtui_image_demo_test main.cpp)
    qt_add_qml_module(qtui_image_demo_test
        URI ImageDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/ImageDemo"
        VERSION 1.0
        QML_FILES Main.qml
        RESOURCES
            demo_image.png
            panda.png
        DEPENDENCIES TARGET
            Qtui::Core
            Qtui::Controls
            Qtui::Image
    )
    target_link_libraries(qtui_image_demo_test PRIVATE
        Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Controls Qtui::Image Qtui::Runner)
    qtui_add_scenario_test(
        NAME image_demo_render_modes
        TARGET qtui_image_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_image_demo.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
