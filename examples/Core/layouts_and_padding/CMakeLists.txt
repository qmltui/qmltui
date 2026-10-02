qt_add_executable(qtui_layouts_and_padding main.cpp)

qt_add_qml_module(qtui_layouts_and_padding
    URI LayoutsAndPadding
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore
)

target_link_libraries(qtui_layouts_and_padding PRIVATE Qt6::Core Qt6::Qml QtuiCore)
