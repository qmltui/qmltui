qt_add_executable(qtui_plotting_dashboard main.cpp)

qt_add_qml_module(qtui_plotting_dashboard
    URI PlottingDashboard
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore
)

target_link_libraries(qtui_plotting_dashboard PRIVATE Qt6::Core Qt6::Qml QtuiCore)
