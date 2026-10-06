qt_add_executable(qtui_progress_bar main.cpp)

qt_add_qml_module(qtui_progress_bar
    URI ProgressBarDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_progress_bar PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

install(TARGETS qtui_progress_bar DESTINATION bin)
