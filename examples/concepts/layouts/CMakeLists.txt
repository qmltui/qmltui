qt_add_executable(qamui_layouts_and_padding main.cpp)

qt_add_qml_module(qamui_layouts_and_padding
    URI LayoutsAndPadding
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_layouts_and_padding PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
