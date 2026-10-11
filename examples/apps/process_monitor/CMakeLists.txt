qt_add_executable(qamui_top_monitor_demo
    main.cpp
    ProcessTableModel.h
    ProcessTableModel.cpp
)

qt_add_qml_module(qamui_top_monitor_demo
    URI TopMonitorDemo
    VERSION 1.0
    QML_FILES
        Main.qml
 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_top_monitor_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Controls)
