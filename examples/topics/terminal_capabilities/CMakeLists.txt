qt_add_executable(qmltui_terminal_capabilities_demo main.cpp)

qt_add_qml_module(qmltui_terminal_capabilities_demo
    URI TerminalCapabilitiesDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES
        TARGET qmltui::core
        TARGET qmltui::controls
)

target_link_libraries(qmltui_terminal_capabilities_demo PRIVATE
    Qt6::Core
    Qt6::Qml
    qmltui::core
    qmltui::controls
)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_terminal_capabilities_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME terminal_capabilities_smoke TARGET qmltui_terminal_capabilities_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
