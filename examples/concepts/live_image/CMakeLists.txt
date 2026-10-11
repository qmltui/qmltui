# Dynamic pictures from C++: an image:// URL served asynchronously by a
# qtui::AsyncImageProvider. Each executable owns the QML module so it runs
# from a shell without an import-path environment variable.
set(live_image_sources frameprovider.h frameprovider.cpp)

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
    qt_add_executable(qtui_live_image_test main.cpp)
    qt_add_qml_module(qtui_live_image_test
        URI LiveImageDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/LiveImageDemo"
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
    qtui_add_scenario_test(
        NAME live_image_large
        TARGET qtui_live_image_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_live_image.js"
        COLUMNS 240
        ROWS 72
        LABELS AgentSmoke
    )
endif()

if(QTUI_BUILD_TESTS)
    qt_add_executable(test_live_image_provider test_frameprovider.cpp frameprovider.cpp frameprovider.h)
    target_link_libraries(test_live_image_provider PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test Qtui::Image)
    add_test(NAME live_image_provider COMMAND test_live_image_provider)
endif()
