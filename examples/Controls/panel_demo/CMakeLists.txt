qt_add_executable(qtui_panel_demo main.cpp)

qt_add_qml_module(qtui_panel_demo
    URI PanelDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET QtuiCore QtuiControls
)

target_link_libraries(qtui_panel_demo PRIVATE Qt6::Core Qt6::Qml QtuiCore QtuiWidgets QtuiControls)
