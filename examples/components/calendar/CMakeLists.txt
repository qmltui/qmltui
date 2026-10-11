qt_add_executable(qamui_calendar_demo main.cpp)

qt_add_qml_module(qamui_calendar_demo
    URI CalendarDemo
    VERSION 1.0
    QML_FILES
        Calendar.qml
        StatusBadge.qml
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_calendar_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Controls)

if(TARGET Qamui::Runner)

    target_link_libraries(qamui_calendar_demo PRIVATE Qamui::Runner)

    qamui_add_scenario(
        NAME calendar_smoke
        TARGET qamui_calendar_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_calendar.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
