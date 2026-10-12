qt_add_executable(qmltui_terminal_view_demo main.cpp)
qt_add_qml_module(qmltui_terminal_view_demo
    URI TerminalViewDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls TARGET qmltui::terminal_host
)
target_link_libraries(qmltui_terminal_view_demo PRIVATE
    Qt6::Core Qt6::Qml qmltui::core qmltui::controls qmltui::terminal_host
)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_terminal_view_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME terminal_view_smoke TARGET qmltui_terminal_view_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_terminal_view.js"
        COLUMNS 160 ROWS 40 LABELS AgentSmoke)
endif()
