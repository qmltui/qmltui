qt_add_executable(qmltui_calendar_demo main.cpp)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../status_badge/StatusBadge.qml"
    "${CMAKE_CURRENT_BINARY_DIR}/StatusBadge.qml"
    COPYONLY
)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/StatusBadge.qml"
    PROPERTIES QT_RESOURCE_ALIAS StatusBadge.qml
)

qt_add_qml_module(qmltui_calendar_demo
    URI CalendarDemo
    VERSION 1.0
    QML_FILES
        Calendar.qml
        "${CMAKE_CURRENT_BINARY_DIR}/StatusBadge.qml"
        Main.qml

    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)

target_link_libraries(qmltui_calendar_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(TARGET qmltui::runner)

    target_link_libraries(qmltui_calendar_demo PRIVATE qmltui::runner)

    qmltui_add_scenario(
        NAME calendar_smoke
        TARGET qmltui_calendar_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_calendar.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
        RECORD components/calendar.cast
        LABELS AgentSmoke
    )
endif()
