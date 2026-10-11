qt_add_executable(qamui_spaceship_game main.cpp)

qt_add_qml_module(qamui_spaceship_game
    URI SpaceshipGame
    VERSION 1.0
    QML_FILES
        Main.qml
        Bullet.qml
        Enemy.qml
        Explosion.qml
        PowerUp.qml
        Star.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_spaceship_game PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
