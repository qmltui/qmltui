qt_add_executable(qtui_animation_demo main.cpp)

qt_add_qml_module(qtui_animation_demo
    URI AnimationDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET QtuiCore
)

target_link_libraries(qtui_animation_demo PRIVATE Qt6::Core Qt6::Qml QtuiCore)
