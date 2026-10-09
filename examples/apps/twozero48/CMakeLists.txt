qt_add_executable(qtui_twozero48 main.cpp)

qt_add_qml_module(qtui_twozero48
    URI TwoZero48
    VERSION 1.0
    QML_FILES
        Main.qml
 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_twozero48 PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

if(TARGET Qtui::TestRunner)
    target_link_libraries(qtui_twozero48 PRIVATE Qtui::TestRunner)
endif()

if(COMMAND qtui_wasm_app)   # browser build, see examples/wasm and docs/guide/webassembly.md
    qtui_wasm_app(qtui_twozero48)
endif()
