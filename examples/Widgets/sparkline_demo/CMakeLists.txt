qt_add_executable(qtui_sparkline_demo main.cpp)

qt_add_qml_module(qtui_sparkline_demo
    URI SparklineDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui TerminalWidgets
)

target_link_libraries(qtui_sparkline_demo PRIVATE Qt6::Core Qt6::Qml Qtui TerminalWidgets)
