qt_add_executable(qamui_bigtext_demo main.cpp)

# BigText is a recipe for a custom C++ item: it is compiled into this example's
# own QML module rather than shipped by the library.
qt_add_qml_module(qamui_bigtext_demo
    URI BigTextDemo
    VERSION 1.0
    SOURCES
        BigText.h
        BigText.cpp
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_bigtext_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
