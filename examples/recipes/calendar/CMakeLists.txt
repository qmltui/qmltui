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
    QML_FILES
        Calendar.qml
        "${CMAKE_CURRENT_BINARY_DIR}/StatusBadge.qml"
        Main.qml

    DEPENDENCIES TARGET QmlTui::Core TARGET QmlTui::Controls
)

target_link_libraries(qmltui_calendar_demo PRIVATE Qt6::Core Qt6::Qml QmlTui::Core QmlTui::Controls)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_calendar_demo)
endif()

if(BUILD_TESTING AND TARGET QmlTui::Runner)

    target_link_libraries(qmltui_calendar_demo PRIVATE QmlTui::Runner)

    qmltui_add_scenario(
        NAME calendar_smoke
        TARGET qmltui_calendar_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_calendar.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
    )
    qmltui_add_scenario(
        NAME calendar_demo
        TARGET qmltui_calendar_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_calendar.js"
        COLUMNS 80
        ROWS 24
        CLOCK REAL
        RECORD components/calendar.cast
    )
endif()
