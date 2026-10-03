qt_add_executable(qtui_terminal_view main.cpp)
qt_add_qml_module(qtui_terminal_view
    URI TerminalViewDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qtui::Core Qtui::TerminalHost
)
target_link_libraries(qtui_terminal_view PRIVATE
    Qt6::Core Qt6::Qml Qtui::Core Qtui::TerminalHost
)

