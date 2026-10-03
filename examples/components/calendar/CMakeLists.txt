qt_add_executable(qtui_calendar main.cpp)

qt_add_qml_module(qtui_calendar
    URI CalendarDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_calendar PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
