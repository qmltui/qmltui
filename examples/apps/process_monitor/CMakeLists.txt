qt_add_executable(qtui_top_monitor_demo
    main.cpp
    ProcessTableModel.h
    ProcessTableModel.cpp
)

qt_add_qml_module(qtui_top_monitor_demo
    URI TopMonitorDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui::Core Qtui::Extras Qtui::Controls
)

target_link_libraries(qtui_top_monitor_demo PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Extras Qtui::Controls)
