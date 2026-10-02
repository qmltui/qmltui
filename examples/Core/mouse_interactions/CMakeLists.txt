qt_add_executable(qtui_mouse_interactions main.cpp)

qt_add_qml_module(qtui_mouse_interactions
    URI MouseInteractions
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore
)

target_link_libraries(qtui_mouse_interactions PRIVATE Qt6::Core Qt6::Qml QtuiCore)
