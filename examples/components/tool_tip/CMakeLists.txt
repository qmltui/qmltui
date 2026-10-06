qt_add_executable(qtui_tool_tip main.cpp)

qt_add_qml_module(qtui_tool_tip
    URI ToolTipDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_tool_tip PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
