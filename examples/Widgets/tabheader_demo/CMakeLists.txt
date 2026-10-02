qt_add_executable(qtui_tabheader_demo
    main.cpp
)

qt_add_qml_module(qtui_tabheader_demo
    URI TabHeaderDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui TerminalWidgets TerminalControls
)

target_link_libraries(qtui_tabheader_demo PRIVATE Qt6::Core Qt6::Qml Qtui TerminalWidgets TerminalControls)
