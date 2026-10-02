qt_add_executable(qtui_bigtext_demo main.cpp)

qt_add_qml_module(qtui_bigtext_demo
    URI BigTextDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui TerminalWidgets
)

target_link_libraries(qtui_bigtext_demo PRIVATE Qt6::Core Qt6::Qml Qtui TerminalWidgets)
