qt_add_executable(qtui_focus_and_keys main.cpp)

qt_add_qml_module(qtui_focus_and_keys
    URI FocusAndKeys
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore
)

target_link_libraries(qtui_focus_and_keys PRIVATE Qt6::Core Qt6::Qml QtuiCore)
