qt_add_library(qtui_custom_item_module STATIC)

qt_add_qml_module(qtui_custom_item_module
    URI CustomItemDemo
    VERSION 1.0
    SOURCES
        progressbar.h
        progressbar.cpp
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_custom_item_module PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
)

qt_add_executable(qtui_custom_item_demo main.cpp)
target_link_libraries(qtui_custom_item_demo PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    qtui_custom_item_module
)

if(TARGET Qtui::TestRunner AND QTUI_BUILD_TESTS)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
    qt_add_executable(qtui_custom_item_demo_test main.cpp)
    target_link_libraries(qtui_custom_item_demo_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::TestRunner
        qtui_custom_item_module
    )
    qtui_add_scenario_test(
        NAME custom_cpp_item_smoke
        TARGET qtui_custom_item_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_custom_item.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
