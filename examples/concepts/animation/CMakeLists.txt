qt_add_executable(qamui_animation_demo main.cpp)

qt_add_qml_module(qamui_animation_demo
    URI AnimationDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_animation_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
