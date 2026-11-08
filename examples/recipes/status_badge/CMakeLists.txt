qt_add_executable(qmltui_status_badge_demo main.cpp)

qt_add_qml_module(qmltui_status_badge_demo
    URI StatusBadgeDemo
    VERSION 1.0
    QML_FILES
        StatusBadge.qml
        Main.qml

    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)

target_link_libraries(qmltui_status_badge_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_status_badge_demo)
endif()

if(BUILD_TESTING AND TARGET qmltui::runner)
    target_link_libraries(qmltui_status_badge_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME status_badge_smoke TARGET qmltui_status_badge_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL)
    qmltui_add_scenario(NAME status_badge_demo TARGET qmltui_status_badge_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK REAL RECORD components/status_badge.cast)
endif()
