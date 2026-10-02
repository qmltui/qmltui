qt_add_executable(qtui_twozero48 main.cpp)

qt_add_qml_module(qtui_twozero48
    URI TwoZero48
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui TerminalWidgets
)

target_link_libraries(qtui_twozero48 PRIVATE Qt6::Core Qt6::Qml Qtui TerminalWidgets)
