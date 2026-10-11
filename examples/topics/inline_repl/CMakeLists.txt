qt_add_executable(qamui_inline_repl_demo main.cpp)
qt_add_qml_module(qamui_inline_repl_demo
    URI InlineRepl
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core
)
target_link_libraries(qamui_inline_repl_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_inline_repl_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME inline_repl_smoke TARGET qamui_inline_repl_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_inline_repl.js"
        COLUMNS 80 ROWS 24 LABELS AgentSmoke)
endif()
