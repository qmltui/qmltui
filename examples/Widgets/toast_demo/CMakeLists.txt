qt_add_executable(qtui_toast_demo main.cpp)

qt_add_qml_module(qtui_toast_demo
    URI ToastDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET QtuiCore QtuiWidgets QtuiControls
)

target_link_libraries(qtui_toast_demo PRIVATE Qt6::Core Qt6::Qml QtuiCore QtuiWidgets QtuiControls)
