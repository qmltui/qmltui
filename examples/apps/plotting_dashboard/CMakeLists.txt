qt_add_executable(qtui_plotting_dashboard
    main.cpp
    BraillePolyline.h
    BraillePolyline.cpp
)

qt_add_qml_module(qtui_plotting_dashboard
    URI PlottingDashboard
    VERSION 1.0
    QML_FILES
        Main.qml
        Chart.qml
        ChartAxes.qml
        BrailleLineSeries.qml

    DEPENDENCIES TARGET Qtui::Core Qtui::Canvas
)

target_link_libraries(qtui_plotting_dashboard PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Canvas
)

if(TARGET Qtui::Testing)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTesting.cmake")

    qt_add_executable(qtui_plotting_dashboard_test
        main_test.cpp
        BraillePolyline.h
        BraillePolyline.cpp
    )

    qt_add_qml_module(qtui_plotting_dashboard_test
        URI PlottingDashboardTest
        VERSION 1.0
        QML_FILES
            Main.qml
            Chart.qml
            ChartAxes.qml
            BrailleLineSeries.qml
        DEPENDENCIES TARGET Qtui::Core Qtui::Canvas
    )

    target_link_libraries(qtui_plotting_dashboard_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::Canvas
        Qtui::Testing
    )

    qtui_add_scenario_test(
        NAME plotting_dashboard_smoke
        TARGET qtui_plotting_dashboard_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_chart.js"
        COLUMNS 120
        ROWS 40
        LABELS AgentSmoke
    )
endif()
