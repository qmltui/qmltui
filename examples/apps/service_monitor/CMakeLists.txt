qt_add_executable(qamui_service_monitor main.cpp)

qt_add_qml_module(qamui_service_monitor
    URI ServiceMonitor
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES
        TARGET Qamui::Core
)

target_link_libraries(qamui_service_monitor PRIVATE
    Qt6::Core
    Qt6::Qml
    Qamui::Core
)

if(TARGET Qamui::Runner)

    qt_add_executable(qamui_service_monitor_test main.cpp)

    qt_add_qml_module(qamui_service_monitor_test
        URI ServiceMonitor
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/ServiceMonitor"
        VERSION 1.0
        QML_FILES Main.qml
        DEPENDENCIES
            TARGET Qamui::Core
    )

    target_link_libraries(qamui_service_monitor_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qamui::Core
        Qamui::Runner
    )

    qamui_add_scenario_test(
        NAME service_monitor_smoke
        TARGET qamui_service_monitor_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_service_monitor.js"
        COLUMNS 72
        ROWS 16
        LABELS AgentSmoke
    )

    # The application must not repaint while nothing changes.
    qamui_add_scenario_test(
        NAME service_monitor_idle_budget
        TARGET qamui_service_monitor_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_idle_budget.js"
        COLUMNS 72
        ROWS 16
        LABELS AgentSmoke
    )
endif()
