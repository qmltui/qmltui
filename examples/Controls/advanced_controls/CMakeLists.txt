qt_add_executable(qtui_advanced_controls main.cpp)

qt_add_qml_module(qtui_advanced_controls
    URI AdvancedControls
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore QtuiControls
)

target_link_libraries(qtui_advanced_controls PRIVATE Qt6::Core Qt6::Qml QtuiCore QtuiControls)
