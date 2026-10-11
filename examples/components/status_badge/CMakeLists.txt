qt_add_executable(qamui_status_badge main.cpp)

qt_add_qml_module(qamui_status_badge
    URI StatusBadgeDemo
    VERSION 1.0
    QML_FILES
        StatusBadge.qml
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_status_badge PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
