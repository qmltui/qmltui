qt_add_executable(qamui_braille_demo main.cpp)

qt_add_qml_module(qamui_braille_demo
    URI BrailleDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES
        TARGET Qamui::Core
        TARGET Qamui::Core
)
