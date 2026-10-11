qt_add_executable(qamui_calendar main.cpp)

qt_add_qml_module(qamui_calendar
    URI CalendarDemo
    VERSION 1.0
    QML_FILES
        Calendar.qml
        StatusBadge.qml
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_calendar PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Controls)

if(TARGET Qamui::Runner)

    qt_add_executable(qamui_calendar_test main.cpp)

    qt_add_qml_module(qamui_calendar_test
        URI CalendarDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/CalendarDemo"
        VERSION 1.0
        QML_FILES
        Calendar.qml
        StatusBadge.qml
            Main.qml
        DEPENDENCIES
 TARGET Qamui::Core Qamui::Controls
    )

    target_link_libraries(qamui_calendar_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qamui::Core
        Qamui::Controls
        Qamui::Controls
        Qamui::Runner
    )

    qamui_add_scenario_test(
        NAME calendar_smoke
        TARGET qamui_calendar_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_calendar.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
