# Dynamic pictures from C++: an image:// URL served asynchronously by a
# qmltui::AsyncImageProvider. The executable owns the QML module so it runs
# from a shell without an import-path environment variable.
set(live_image_sources frameprovider.h frameprovider.cpp)

qt_add_executable(qmltui_live_image_demo main.cpp)
qt_add_qml_module(qmltui_live_image_demo
    URI LiveImageDemo
    VERSION 1.0
    SOURCES ${live_image_sources}
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls TARGET qmltui::image
)
target_link_libraries(qmltui_live_image_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml qmltui::core qmltui::controls qmltui::image)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_live_image_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(
        NAME live_image_smoke
        TARGET qmltui_live_image_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_live_image.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
    qmltui_add_scenario(
        NAME live_image_large
        TARGET qmltui_live_image_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_live_image.js"
        COLUMNS 240
        ROWS 72
        LABELS AgentSmoke
    )
endif()

if(QMLTUI_BUILD_TESTS)
    qt_add_executable(test_live_image_provider test_frameprovider.cpp frameprovider.cpp frameprovider.h)
    target_link_libraries(test_live_image_provider PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test qmltui::image)
    add_test(NAME live_image_provider COMMAND test_live_image_provider)
endif()
