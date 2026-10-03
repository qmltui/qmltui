qt_add_executable(qtui_industrial_plc_hmi main.cpp)

set_source_files_properties(HmiTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(qtui_industrial_plc_hmi
    URI IndustrialPlcHmi
    VERSION 1.0
    QML_FILES
        Main.qml
        HmiTheme.qml
        Gauge.qml
        ActuatorLed.qml
        HeaderPanel.qml
        ConfirmDialog.qml
        NavTabBar.qml
        AlarmsView.qml
        DiagnosticsView.qml
        SettingsView.qml

    DEPENDENCIES TARGET Qtui::Core Qtui::Canvas
)

target_link_libraries(qtui_industrial_plc_hmi PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Canvas)
