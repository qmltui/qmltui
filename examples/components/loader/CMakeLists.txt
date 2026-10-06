qt_add_executable(qtui_loader_demo main.cpp)

qt_add_qml_module(qtui_loader_demo
    URI LoaderDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_loader_demo PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Controls
)
