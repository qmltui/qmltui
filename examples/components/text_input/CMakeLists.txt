qt_add_executable(qamui_text_input_demo main.cpp)

qt_add_qml_module(qamui_text_input_demo
    URI TextInputDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_text_input_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_text_input_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME text_input_smoke TARGET qamui_text_input_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME text_input_demo TARGET qamui_text_input_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/text_input.cast LABELS Cast)
endif()
