qt_add_executable(qtui_snake_game main.cpp)

qt_add_qml_module(qtui_snake_game
    URI SnakeGame
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES
        TARGET Qtui::Core
)

target_link_libraries(qtui_snake_game PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

if(TARGET Qtui::Runner)
    target_link_libraries(qtui_snake_game PRIVATE Qtui::Runner)
endif()

if(COMMAND qtui_wasm_app)   # browser build, see examples/wasm and docs/guide/webassembly.md
    qtui_wasm_app(qtui_snake_game)
endif()
