qt_add_executable(qamui_state_transitions_demo main.cpp)

qt_add_qml_module(qamui_state_transitions_demo
    URI StateTransitionsDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_state_transitions_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
