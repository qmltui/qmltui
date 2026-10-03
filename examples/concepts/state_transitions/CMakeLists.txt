qt_add_executable(qtui_state_transitions_demo main.cpp)

qt_add_qml_module(qtui_state_transitions_demo
    URI StateTransitionsDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_state_transitions_demo PRIVATE Qt6::Core Qt6::Qml Qtui::Core)
