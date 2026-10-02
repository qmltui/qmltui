qt_add_executable(qtui_braille_demo main.cpp)

qt_add_qml_module(qtui_braille_demo
    URI BrailleDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore
)

target_link_libraries(qtui_braille_demo PRIVATE Qt6::Core Qt6::Qml QtuiCore)
