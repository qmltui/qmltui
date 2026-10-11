qt_add_executable(qamui_rectangle main.cpp)

qt_add_qml_module(qamui_rectangle
    URI RectangleDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES
        TARGET Qamui::Core
)

target_link_libraries(qamui_rectangle PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

