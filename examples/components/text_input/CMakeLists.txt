qt_add_executable(qtui_text_input main.cpp)

qt_add_qml_module(qtui_text_input
    URI TextInputDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_text_input PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

