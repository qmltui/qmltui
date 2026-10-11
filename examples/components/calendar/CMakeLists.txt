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

if(TARGET Qtui::Runner)

    qt_add_executable(qtui_calendar_test main.cpp)

    qt_add_qml_module(qtui_calendar_test
        URI CalendarDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/CalendarDemo"
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
        Qtui::Runner
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
