qt_add_executable(qamui_snake_game main.cpp)

qt_add_qml_module(qamui_snake_game
    URI SnakeGame
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES
        TARGET Qamui::Core
)

target_link_libraries(qamui_snake_game PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_snake_game PRIVATE Qamui::Runner)
endif()

if(COMMAND qamui_wasm_app)   # browser build, see examples/wasm and docs/guide/webassembly.md
    qamui_wasm_app(qamui_snake_game)
endif()
