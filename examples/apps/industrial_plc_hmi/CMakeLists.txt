set_source_files_properties(HmiTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(PlcTypes.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(qtui_industrial_plc_hmi main.cpp)

qt_add_qml_module(qtui_industrial_plc_hmi
    URI IndustrialPlcHmi
    VERSION 1.0
    QML_FILES
        Main.qml
        HmiTheme.qml
        PlcTypes.qml
        Gauge.qml
        ActuatorLed.qml
        HeaderPanel.qml
        ConfirmDialog.qml
        NavTabBar.qml
        AlarmsView.qml
        DiagnosticsView.qml
        SettingsView.qml

    RESOURCES
        test_plc.js

    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
        TARGET Qtui::Canvas
        TARGET Qtui::Extras
)

target_link_libraries(qtui_industrial_plc_hmi PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Controls
    Qtui::Canvas
    Qtui::Extras
)
