qt_add_executable(qamui_text_demo main.cpp)

qt_add_qml_module(qamui_text_demo
    URI TextDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_text_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_text_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(
        NAME text_demo_spans
        TARGET qamui_text_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_spans.js"
        COLUMNS 80 ROWS 24
        LABELS AgentSmoke
    )
endif()
