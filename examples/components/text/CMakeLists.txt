qt_add_executable(qtui_text main.cpp)

qt_add_qml_module(qtui_text
    URI TextDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_text PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

