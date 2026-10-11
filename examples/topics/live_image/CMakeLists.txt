# Dynamic pictures from C++: an image:// URL served asynchronously by a
# qamui::AsyncImageProvider. Each executable owns the QML module so it runs
# from a shell without an import-path environment variable.
set(live_image_sources frameprovider.h frameprovider.cpp)

qt_add_executable(qamui_live_image_demo main.cpp)
qt_add_qml_module(qamui_live_image_demo
    URI LiveImageDemo
    VERSION 1.0
    SOURCES ${live_image_sources}
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls TARGET Qamui::Image
)
target_link_libraries(qamui_live_image_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qamui::Core Qamui::Controls Qamui::Image)

if(TARGET Qamui::Runner AND QAMUI_BUILD_TESTS)
    qt_add_executable(qamui_live_image_demo_test main.cpp)
    qt_add_qml_module(qamui_live_image_demo_test
        URI LiveImageDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/LiveImageDemo"
        VERSION 1.0
        SOURCES ${live_image_sources}
        QML_FILES Main.qml
        DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls TARGET Qamui::Image
    )
    target_link_libraries(qamui_live_image_demo_test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qamui::Core Qamui::Controls Qamui::Image Qamui::Runner)
    qamui_add_scenario_test(
        NAME live_image_smoke
        TARGET qamui_live_image_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_live_image.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
    qamui_add_scenario_test(
        NAME live_image_large
        TARGET qamui_live_image_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_live_image.js"
        COLUMNS 240
        ROWS 72
        LABELS AgentSmoke
    )
endif()

if(QAMUI_BUILD_TESTS)
    qt_add_executable(test_live_image_provider test_frameprovider.cpp frameprovider.cpp frameprovider.h)
    target_link_libraries(test_live_image_provider PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test Qamui::Image)
    add_test(NAME live_image_provider COMMAND test_live_image_provider)
endif()
