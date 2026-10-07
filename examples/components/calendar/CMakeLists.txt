qt_add_executable(qtui_calendar main.cpp)

qt_add_qml_module(qtui_calendar
    URI CalendarDemo
    VERSION 1.0
    QML_FILES
        Calendar.qml
        StatusBadge.qml
        Main.qml

 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_calendar PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::Controls)

if(TARGET Qtui::TestRunner)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")

    qt_add_executable(qtui_calendar_test main_test.cpp)

    qt_add_qml_module(qtui_calendar_test
        URI CalendarDemoTest
        VERSION 1.0
        QML_FILES
        Calendar.qml
        StatusBadge.qml
            Main.qml
        DEPENDENCIES
 TARGET Qtui::Core Qtui::Controls
    )

    target_link_libraries(qtui_calendar_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::Controls
        Qtui::Controls
        Qtui::TestRunner
    )

    qtui_add_scenario_test(
        NAME calendar_smoke
        TARGET qtui_calendar_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_calendar.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
