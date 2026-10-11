# Each executable owns the QML module (as table_view does) so Qt writes the
# build tree's QML import path into a qt.conf beside it and the binary runs
# from Qt Creator or a shell without an import-path environment variable.
qt_add_executable(qtui_custom_item_demo main.cpp)
qt_add_qml_module(qtui_custom_item_demo
    URI CustomItemDemo
    VERSION 1.0
    SOURCES progressbar.h progressbar.cpp
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qtui::Core
)
target_link_libraries(qtui_custom_item_demo PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

if(TARGET Qtui::Runner AND QTUI_BUILD_TESTS)
    qt_add_executable(qtui_custom_item_demo_test main.cpp)
    qt_add_qml_module(qtui_custom_item_demo_test
        URI CustomItemDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/CustomItemDemo"
        VERSION 1.0
        SOURCES progressbar.h progressbar.cpp
        QML_FILES Main.qml
        DEPENDENCIES TARGET Qtui::Core
    )
    target_link_libraries(qtui_custom_item_demo_test PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Runner)
    qtui_add_scenario_test(
        NAME custom_cpp_item_smoke
        TARGET qtui_custom_item_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_custom_item.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
