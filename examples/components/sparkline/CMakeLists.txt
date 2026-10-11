qt_add_executable(qamui_sparkline_demo main.cpp)

qt_add_qml_module(qamui_sparkline_demo
    URI SparklineDemo
    VERSION 1.0
    QML_FILES
        Main.qml
 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_sparkline_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
