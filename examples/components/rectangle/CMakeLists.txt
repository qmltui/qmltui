qt_add_executable(qtui_rectangle main.cpp)

qt_add_qml_module(qtui_rectangle
    URI RectangleDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES
        TARGET Qtui::Core
)

target_link_libraries(qtui_rectangle PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

