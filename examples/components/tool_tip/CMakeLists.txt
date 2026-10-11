qt_add_executable(qamui_tool_tip main.cpp)

qt_add_qml_module(qamui_tool_tip
    URI ToolTipDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_tool_tip PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
