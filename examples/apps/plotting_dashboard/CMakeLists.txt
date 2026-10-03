qt_add_executable(qtui_plotting_dashboard main.cpp)

qt_add_qml_module(qtui_plotting_dashboard
    URI PlottingDashboard
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core Qtui::Canvas
)

target_link_libraries(qtui_plotting_dashboard PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Canvas)
