cmake_minimum_required(VERSION 3.20)

qt_add_executable(agent_stream_demo main.cpp)

qt_add_qml_module(agent_stream_demo
    URI AgentStreamDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(agent_stream_demo PRIVATE Qt6::Core Qt6::Qml Qtui::Core)
