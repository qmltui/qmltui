qt_add_executable(qamui_ansi_styling_demo main.cpp)

qt_add_qml_module(qamui_ansi_styling_demo
    URI StylingAndAnsi
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_ansi_styling_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_ansi_styling_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME ansi_styling_smoke TARGET qamui_ansi_styling_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
