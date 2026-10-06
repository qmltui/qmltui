qt_add_executable(qtui_system_dashboard main.cpp)

qt_add_qml_module(qtui_system_dashboard
    URI SystemDashboardApp
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_system_dashboard PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

