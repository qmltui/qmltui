qt_add_executable(qtui_custom_item_demo main.cpp progressbar.h progressbar.cpp)

qt_add_qml_module(qtui_custom_item_demo
    URI CustomItemDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui
)

target_link_libraries(qtui_custom_item_demo PRIVATE Qt6::Core Qt6::Qml Qtui)
