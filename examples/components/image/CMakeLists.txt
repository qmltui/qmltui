qt_add_executable(qtui_image_demo main.cpp)

qt_add_qml_module(qtui_image_demo
    URI ImageDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        demo_image.png
        image2.jpg
        panda.png
        map.png
        qt_logo_icon_248726.webp
        
    DEPENDENCIES TARGET
        Qtui::Core
        Qtui::Controls
        Qtui::Image
)

target_link_libraries(qtui_image_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Controls Qtui::Image)

if(TARGET Qtui::TestRunner AND QTUI_BUILD_TESTS)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
    qt_add_executable(qtui_image_demo_test main_test.cpp)
    qt_add_qml_module(qtui_image_demo_test
        URI ImageDemoTest
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
        Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Controls Qtui::Image Qtui::TestRunner)
    qtui_add_scenario_test(
        NAME image_demo_render_modes
        TARGET qtui_image_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_image_demo.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
