cmake_minimum_required(VERSION 3.16)

qt_add_executable(qtui_popup_demo main.cpp)

qt_add_qml_module(qtui_popup_demo
    URI PopupDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore
)

target_link_libraries(qtui_popup_demo PRIVATE Qt6::Core Qt6::Qml QtuiCore)
