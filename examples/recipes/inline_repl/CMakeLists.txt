qt_add_executable(qmltui_inline_repl_demo main.cpp)
qt_add_qml_module(qmltui_inline_repl_demo
    URI InlineRepl
    QML_FILES Main.qml
    DEPENDENCIES TARGET QmlTui::Core TARGET QmlTui::Controls
)
target_link_libraries(qmltui_inline_repl_demo PRIVATE Qt6::Core Qt6::Qml QmlTui::Core QmlTui::Controls)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_inline_repl_demo)
endif()

if(BUILD_TESTING AND TARGET QmlTui::Runner)
    target_link_libraries(qmltui_inline_repl_demo PRIVATE QmlTui::Runner)
    qmltui_add_scenario(NAME inline_repl_smoke TARGET qmltui_inline_repl_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_inline_repl.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL)
    qmltui_add_scenario(NAME inline_repl_demo TARGET qmltui_inline_repl_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 84 ROWS 20 CLOCK REAL RECORD components/inline_mode.cast)
endif()
