qt_add_executable(qamui_radio_button main.cpp)

qt_add_qml_module(qamui_radio_button
    URI RadioButtonDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_radio_button PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
