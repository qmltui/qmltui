qt_add_executable(qmltui_status_badge_demo main.cpp)

qt_add_qml_module(qmltui_status_badge_demo
    URI StatusBadgeDemo
    QML_FILES
        StatusBadge.qml
        Main.qml

    DEPENDENCIES TARGET QmlTui::Core TARGET QmlTui::Controls
)

target_link_libraries(qmltui_status_badge_demo PRIVATE Qt6::Core Qt6::Qml QmlTui::Core QmlTui::Controls)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_status_badge_demo)
endif()

if(BUILD_TESTING AND TARGET QmlTui::Runner)
    target_link_libraries(qmltui_status_badge_demo PRIVATE QmlTui::Runner)
    qmltui_add_scenario(NAME status_badge_smoke TARGET qmltui_status_badge_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL)
    qmltui_add_scenario(NAME status_badge_demo TARGET qmltui_status_badge_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK REAL RECORD components/status_badge.cast)
endif()
