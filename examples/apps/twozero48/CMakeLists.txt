qt_add_executable(qamui_twozero48 main.cpp)

qt_add_qml_module(qamui_twozero48
    URI TwoZero48
    VERSION 1.0
    QML_FILES
        Main.qml
 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_twozero48 PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_twozero48 PRIVATE Qamui::Runner)
endif()

if(COMMAND qamui_wasm_app)   # browser build, see examples/wasm and docs/guide/webassembly.md
    qamui_wasm_app(qamui_twozero48)
endif()
