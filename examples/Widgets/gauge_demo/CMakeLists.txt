qt_add_executable(qtui_gauge_demo
    main.cpp
)

qt_add_qml_module(qtui_gauge_demo
    URI GaugeDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET QtuiCore QtuiWidgets QtuiControls
)

target_link_libraries(qtui_gauge_demo PRIVATE Qt6::Core Qt6::Qml QtuiCore QtuiWidgets QtuiControls)
