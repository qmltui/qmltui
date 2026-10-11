qt_add_executable(qamui_terminal_view_demo main.cpp)
qt_add_qml_module(qamui_terminal_view_demo
    URI TerminalViewDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls TARGET Qamui::TerminalHost
)
target_link_libraries(qamui_terminal_view_demo PRIVATE
    Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::TerminalHost
)

if(TARGET Qamui::Runner)
    qt_add_executable(qamui_terminal_view_demo_test main.cpp)
    target_link_libraries(qamui_terminal_view_demo_test PRIVATE
        Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::TerminalHost Qamui::Runner)
    target_compile_definitions(qamui_terminal_view_demo_test PRIVATE
        QAMUI_TERMINAL_VIEW_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Main.qml")
    qamui_add_scenario_test(NAME terminal_view_smoke TARGET qamui_terminal_view_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_terminal_view.js"
        COLUMNS 160 ROWS 40 LABELS AgentSmoke)
endif()
