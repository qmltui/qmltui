cmake_minimum_required(VERSION 3.16)

qt_add_executable(qamui_popup_demo main.cpp)

qt_add_qml_module(qamui_popup_demo
    URI PopupDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_popup_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
