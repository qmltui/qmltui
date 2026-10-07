qt_add_executable(qtui_cyber_audio main.cpp)

qt_add_qml_module(qtui_cyber_audio
    URI CyberAudio
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
)

target_link_libraries(qtui_cyber_audio PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Controls
)

if(TARGET Qtui::Testing)
    target_link_libraries(qtui_cyber_audio PRIVATE Qtui::Testing)
endif()
