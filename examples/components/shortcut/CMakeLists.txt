qt_add_executable(qamui_shortcut_demo main.cpp)
qt_add_qml_module(qamui_shortcut_demo
    URI ShortcutDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls
)
target_link_libraries(qamui_shortcut_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_shortcut_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME shortcut_smoke TARGET qamui_shortcut_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 100 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME shortcut_demo TARGET qamui_shortcut_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 100 ROWS 24 CLOCK VIRTUAL RECORD components/shortcut.cast LABELS Cast)
endif()
