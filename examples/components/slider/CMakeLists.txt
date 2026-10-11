qt_add_executable(qamui_slider main.cpp)

qt_add_qml_module(qamui_slider
    URI SliderDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_slider PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
