qt_add_executable(qamui_plotting_dashboard main.cpp)

qt_add_qml_module(qamui_plotting_dashboard
    URI PlottingDashboard
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_plotting_dashboard PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Core)
