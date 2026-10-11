qt_add_executable(qamui_focus_and_keys main.cpp)

qt_add_qml_module(qamui_focus_and_keys
    URI FocusAndKeys
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_focus_and_keys PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
