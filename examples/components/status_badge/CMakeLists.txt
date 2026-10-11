qt_add_executable(qamui_status_badge_demo main.cpp)

qt_add_qml_module(qamui_status_badge_demo
    URI StatusBadgeDemo
    VERSION 1.0
    QML_FILES
        StatusBadge.qml
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_status_badge_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_status_badge_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME status_badge_smoke TARGET qamui_status_badge_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME status_badge_demo TARGET qamui_status_badge_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/status_badge.cast LABELS Cast)
endif()
