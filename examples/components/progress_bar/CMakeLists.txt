qt_add_executable(qamui_progress_bar main.cpp)

qt_add_qml_module(qamui_progress_bar
    URI ProgressBarDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_progress_bar PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

install(TARGETS qamui_progress_bar DESTINATION bin)
