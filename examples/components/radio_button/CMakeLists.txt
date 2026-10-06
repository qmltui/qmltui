qt_add_executable(qtui_radio_button main.cpp)

qt_add_qml_module(qtui_radio_button
    URI RadioButtonDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_radio_button PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
