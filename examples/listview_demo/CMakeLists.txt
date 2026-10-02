qt_add_executable(qtui_listview_demo main.cpp)

qt_add_qml_module(qtui_listview_demo
    URI ListViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui
)

target_link_libraries(qtui_listview_demo PRIVATE Qt6::Core Qt6::Qml Qtui)
