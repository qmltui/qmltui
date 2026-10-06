qt_add_executable(qtui_bigtext_demo main.cpp)

# BigText is a recipe for a custom C++ item: it is compiled into this example's
# own QML module rather than shipped by the library.
qt_add_qml_module(qtui_bigtext_demo
    URI BigTextDemo
    VERSION 1.0
    SOURCES
        BigText.h
        BigText.cpp
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_bigtext_demo PRIVATE Qt6::Core Qt6::Qml Qtui::Core)
