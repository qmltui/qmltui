qt_add_executable(qmltui_text_demo main.cpp)

qt_add_qml_module(qmltui_text_demo
    URI TextDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET qmltui::core
)

target_link_libraries(qmltui_text_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_text_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(
        NAME text_demo_spans
        TARGET qmltui_text_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_spans.js"
        COLUMNS 80 ROWS 24
        LABELS AgentSmoke
    )
endif()
