qt_add_qml_module(TerminalWidgets
    URI Terminal.Widgets
    VERSION 1.0
    SOURCES
        Sparkline.h Sparkline.cpp
        BigText.h BigText.cpp
        TableView.h TableView.cpp
        TreeView.h TreeView.cpp
        ToastManager.h ToastManager.cpp
        TabHeader.h TabHeader.cpp
        Gauge.h Gauge.cpp
        Paragraph.h Paragraph.cpp
    DEPENDENCIES TARGET Qtui
)

target_include_directories(TerminalWidgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(TerminalWidgets PUBLIC Qtui PRIVATE Qt6::Core Qt6::Qml util)
