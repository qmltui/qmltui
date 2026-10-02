qt_add_executable(qtui_treeview_demo main.cpp)

qt_add_qml_module(qtui_treeview_demo
    URI TreeViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET QtuiCore QtuiWidgets
)

target_link_libraries(qtui_treeview_demo PRIVATE Qt6::Core Qt6::Qml QtuiCore QtuiWidgets)
