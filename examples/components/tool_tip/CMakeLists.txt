qt_add_executable(qamui_tool_tip_demo main.cpp)

qt_add_qml_module(qamui_tool_tip_demo
    URI ToolTipDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_tool_tip_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_tool_tip_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME tool_tip_smoke TARGET qamui_tool_tip_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME tool_tip_demo TARGET qamui_tool_tip_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/tool_tip.cast LABELS Cast)
endif()
