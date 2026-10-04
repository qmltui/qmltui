qt_add_executable(qtui_snake_game main.cpp)

qt_add_qml_module(qtui_snake_game
    URI SnakeGame
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES
        Qtui
        Qtui.Canvas
)

target_link_libraries(qtui_snake_game PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Canvas)

if(TARGET Qtui::Testing)
    target_link_libraries(qtui_snake_game PRIVATE Qtui::Testing)
endif()
