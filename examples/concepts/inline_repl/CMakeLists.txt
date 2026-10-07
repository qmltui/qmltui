qt_add_executable(qtui_inline_repl main.cpp)
qt_add_qml_module(qtui_inline_repl
    URI InlineRepl
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qtui::Core
)
target_link_libraries(qtui_inline_repl PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

if(TARGET Qtui::TestRunner)
    qt_add_executable(qtui_inline_repl_test main.cpp)
 target_link_libraries(qtui_inline_repl_test PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::TestRunner)
    target_compile_definitions(qtui_inline_repl_test PRIVATE
        QTUI_INLINE_REPL_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Main.qml")
    include("${Qtui_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
    qtui_add_scenario_test(NAME inline_repl_smoke TARGET qtui_inline_repl_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_inline_repl.js"
        COLUMNS 80 ROWS 24 LABELS AgentSmoke)
endif()
