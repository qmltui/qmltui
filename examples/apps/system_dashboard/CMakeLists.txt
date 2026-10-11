qt_add_executable(qamui_system_dashboard main.cpp)

qt_add_qml_module(qamui_system_dashboard
    URI SystemDashboardApp
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_system_dashboard PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

