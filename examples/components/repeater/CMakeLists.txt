qt_add_executable(qamui_repeater_demo main.cpp)
qt_add_qml_module(qamui_repeater_demo
    URI RepeaterDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls
)
target_link_libraries(qamui_repeater_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_repeater_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME repeater_smoke TARGET qamui_repeater_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 100 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME repeater_demo TARGET qamui_repeater_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 100 ROWS 24 CLOCK VIRTUAL RECORD components/repeater.cast LABELS Cast)
endif()
