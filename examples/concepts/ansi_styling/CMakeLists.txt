qt_add_executable(qamui_styling_and_ansi main.cpp)

qt_add_qml_module(qamui_styling_and_ansi
    URI StylingAndAnsi
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_styling_and_ansi PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
