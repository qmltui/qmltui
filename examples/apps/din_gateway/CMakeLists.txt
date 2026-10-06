qt_add_executable(qtui_din_gateway
    main.cpp
    GatewayBackend.cpp
    GatewayBackend.h
)

set_source_files_properties(GatewayTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(qtui_din_gateway
    URI DinGatewayApp
    VERSION 1.0
    SOURCES
        GatewayBackend.cpp
        GatewayBackend.h
    QML_FILES
        Main.qml
        GatewayTheme.qml
        HeaderPanel.qml
        NavTabBar.qml
        SensorsOverview.qml
        TrafficMonitor.qml
        SystemNetwork.qml
        ConfigView.qml

 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_din_gateway PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

if(QTUI_INSTALL_EXAMPLES)
    install(TARGETS qtui_din_gateway RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
