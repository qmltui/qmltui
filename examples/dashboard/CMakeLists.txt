qt_add_executable(qtui_dashboard_demo main.cpp)

qt_add_qml_module(qtui_dashboard_demo
    URI DashboardDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui
)

target_link_libraries(qtui_dashboard_demo PRIVATE Qt6::Core Qt6::Qml Qtui)
