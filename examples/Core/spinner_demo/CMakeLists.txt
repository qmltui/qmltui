cmake_minimum_required(VERSION 3.16)

qt_add_executable(qtui_spinner_demo main.cpp)

qt_add_qml_module(qtui_spinner_demo
    URI SpinnerDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore QtuiControls
)

target_link_libraries(qtui_spinner_demo PRIVATE Qt6::Core Qt6::Qml QtuiCore QtuiControls)
