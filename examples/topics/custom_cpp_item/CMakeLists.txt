# Each executable owns the QML module (as table_view does) so Qt writes the
# build tree's QML import path into a qt.conf beside it and the binary runs
# from Qt Creator or a shell without an import-path environment variable.
qt_add_executable(qamui_custom_cpp_item_demo main.cpp)
qt_add_qml_module(qamui_custom_cpp_item_demo
    URI CustomItemDemo
    VERSION 1.0
    SOURCES progressbar.h progressbar.cpp
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core
)
target_link_libraries(qamui_custom_cpp_item_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_custom_cpp_item_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(
        NAME custom_cpp_item_smoke
        TARGET qamui_custom_cpp_item_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_custom_item.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
