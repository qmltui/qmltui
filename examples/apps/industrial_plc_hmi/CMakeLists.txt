set_source_files_properties(HmiTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(PlcTypes.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(qtui_industrial_plc_hmi_module STATIC)

qt_add_qml_module(qtui_industrial_plc_hmi_module
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

    DEPENDENCIES TARGET Qtui::Core Qtui::Canvas Qtui::Extras
)

target_link_libraries(qtui_industrial_plc_hmi_module PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Canvas
    Qtui::Extras
)

qt_add_executable(qtui_industrial_plc_hmi main.cpp)
target_link_libraries(qtui_industrial_plc_hmi PRIVATE
    Qt6::Core
    Qt6::Qml
    qtui_industrial_plc_hmi_moduleplugin
)

if(TARGET Qtui::Testing)
    qt_add_executable(qtui_industrial_plc_hmi_test_runner main_test_runner.cpp)
    target_link_libraries(qtui_industrial_plc_hmi_test_runner PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::Testing
        qtui_industrial_plc_hmi_moduleplugin
    )
    add_executable(qtui_industrial_plc_hmi_test_driver ALIAS qtui_industrial_plc_hmi_test_runner)
endif()
