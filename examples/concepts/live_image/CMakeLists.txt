# Dynamic pictures from C++: Image.image bound to a QImage property, and an
# image:// URL served by a qtui::ImageProvider. Each executable owns the QML
# module so it runs from a shell without an import-path environment variable.
set(live_image_sources framesource.h framesource.cpp frameprovider.h frameprovider.cpp)

qt_add_executable(qtui_live_image main.cpp)
qt_add_qml_module(qtui_live_image
    URI LiveImageDemo
    VERSION 1.0
    SOURCES ${live_image_sources}
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qtui::Core TARGET Qtui::Controls TARGET Qtui::Image
)
target_link_libraries(qtui_live_image PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Controls Qtui::Image)

if(TARGET Qtui::Runner AND QTUI_BUILD_TESTS)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiRunner.cmake")
    qt_add_executable(qtui_live_image_test main_test.cpp)
    qt_add_qml_module(qtui_live_image_test
        URI LiveImageTest
        VERSION 1.0
        SOURCES ${live_image_sources}
        QML_FILES Main.qml
        DEPENDENCIES TARGET Qtui::Core TARGET Qtui::Controls TARGET Qtui::Image
    )
    target_link_libraries(qtui_live_image_test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Controls Qtui::Image Qtui::Runner)
    qtui_add_scenario_test(
        NAME live_image_smoke
        TARGET qtui_live_image_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_live_image.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
