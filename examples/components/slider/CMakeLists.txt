qt_add_executable(qtui_slider main.cpp)

qt_add_qml_module(qtui_slider
    URI SliderDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_slider PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
