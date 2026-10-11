qt_add_executable(qamui_dialog_demo main.cpp)

qt_add_qml_module(qamui_dialog_demo
    URI DialogDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_dialog_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_dialog_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME dialog_smoke TARGET qamui_dialog_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME dialog_demo TARGET qamui_dialog_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/dialog.cast LABELS Cast)
endif()
