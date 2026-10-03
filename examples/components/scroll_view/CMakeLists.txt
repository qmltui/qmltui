qt_add_executable(qtui_scroll_view main.cpp)

qt_add_qml_module(qtui_scroll_view
    URI ScrollViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_scroll_view PRIVATE Qt6::Core Qt6::Qml Qtui::Core)
