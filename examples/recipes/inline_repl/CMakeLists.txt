qt_add_executable(qmltui_inline_repl_demo main.cpp)
qt_add_qml_module(qmltui_inline_repl_demo
    URI InlineRepl
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)
target_link_libraries(qmltui_inline_repl_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_inline_repl_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME inline_repl_smoke TARGET qmltui_inline_repl_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_inline_repl.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
    qmltui_add_scenario(NAME inline_repl_demo TARGET qmltui_inline_repl_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 84 ROWS 20 CLOCK VIRTUAL RECORD components/inline_mode.cast LABELS Cast)
endif()
