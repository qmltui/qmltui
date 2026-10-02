qt_add_executable(qtui_playground main.cpp)

qt_add_qml_module(qtui_playground
    URI Playground
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore
)

target_link_libraries(qtui_playground PRIVATE Qt6::Core Qt6::Qml QtuiCore)
