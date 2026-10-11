qt_add_executable(qtui_terminal_view main.cpp)
qt_add_qml_module(qtui_terminal_view
    URI TerminalViewDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qtui::Core TARGET Qtui::Controls TARGET Qtui::TerminalHost
)
target_link_libraries(qtui_terminal_view PRIVATE
    Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::TerminalHost
)

if(TARGET Qtui::Runner)
    qt_add_executable(qtui_terminal_view_test main.cpp)
    target_link_libraries(qtui_terminal_view_test PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::TerminalHost Qtui::Runner)
    target_compile_definitions(qtui_terminal_view_test PRIVATE
        QTUI_TERMINAL_VIEW_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Main.qml")
    qtui_add_scenario_test(NAME terminal_view_smoke TARGET qtui_terminal_view_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_terminal_view.js"
        COLUMNS 160 ROWS 40 LABELS AgentSmoke)
endif()
