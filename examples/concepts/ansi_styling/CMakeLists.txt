qt_add_executable(qtui_styling_and_ansi main.cpp)

qt_add_qml_module(qtui_styling_and_ansi
    URI StylingAndAnsi
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_styling_and_ansi PRIVATE Qt6::Core Qt6::Qml Qtui::Core)
