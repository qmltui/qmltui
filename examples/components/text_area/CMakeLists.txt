qt_add_executable(qamui_text_area main.cpp)

qt_add_qml_module(qamui_text_area
    URI TextAreaDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_text_area PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
