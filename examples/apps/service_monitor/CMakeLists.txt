qt_add_executable(qtui_service_monitor main.cpp)

qt_add_qml_module(qtui_service_monitor
    URI ServiceMonitor
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES
        TARGET Qtui::Core
)

target_link_libraries(qtui_service_monitor PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
)

if(TARGET Qtui::TestRunner)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")

    qt_add_executable(qtui_service_monitor_test main_test.cpp)

    qt_add_qml_module(qtui_service_monitor_test
        URI ServiceMonitorTest
        VERSION 1.0
        QML_FILES Main.qml
        DEPENDENCIES
            TARGET Qtui::Core
    )

    target_link_libraries(qtui_service_monitor_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::TestRunner
    )

    qtui_add_scenario_test(
        NAME service_monitor_smoke
        TARGET qtui_service_monitor_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_service_monitor.js"
        COLUMNS 72
        ROWS 16
        LABELS AgentSmoke
    )

    # The application must not repaint while nothing changes.
    qtui_add_scenario_test(
        NAME service_monitor_idle_budget
        TARGET qtui_service_monitor_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_idle_budget.js"
        COLUMNS 72
        ROWS 16
        LABELS AgentSmoke
    )
endif()
