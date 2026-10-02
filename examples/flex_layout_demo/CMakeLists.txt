cmake_minimum_required(VERSION 3.16)

qt_add_executable(qtui_flex_layout_demo main.cpp)

qt_add_qml_module(qtui_flex_layout_demo
    URI FlexLayoutDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui
)

target_link_libraries(qtui_flex_layout_demo PRIVATE Qt6::Core Qt6::Qml Qtui)
