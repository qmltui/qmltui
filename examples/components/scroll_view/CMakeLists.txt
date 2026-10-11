qt_add_executable(qamui_scroll_view main.cpp)

qt_add_qml_module(qamui_scroll_view
    URI ScrollViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_scroll_view PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
