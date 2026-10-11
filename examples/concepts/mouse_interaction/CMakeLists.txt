qt_add_executable(qamui_mouse_interactions main.cpp)

qt_add_qml_module(qamui_mouse_interactions
    URI MouseInteractions
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_mouse_interactions PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
