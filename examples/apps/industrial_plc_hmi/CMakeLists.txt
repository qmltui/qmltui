set_source_files_properties(HmiTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(PlcTypes.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(plc_qml_files
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
)

qt_add_executable(qtui_industrial_plc_hmi main.cpp)

qt_add_qml_module(qtui_industrial_plc_hmi
    URI IndustrialPlcHmi
    VERSION 1.0
    QML_FILES ${plc_qml_files}

    RESOURCES
        test_plc.js

    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
        TARGET Qtui::Core
        TARGET Qtui::Controls
)

target_link_libraries(qtui_industrial_plc_hmi PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Controls
    Qtui::Core
    Qtui::Controls
)

if(TARGET Qtui::Testing)
    target_link_libraries(qtui_industrial_plc_hmi PRIVATE Qtui::Testing)
endif()
