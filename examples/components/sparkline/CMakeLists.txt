qt_add_executable(qmltui_sparkline_demo main.cpp)

qt_add_qml_module(qmltui_sparkline_demo
    URI SparklineDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)

target_link_libraries(qmltui_sparkline_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_sparkline_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME sparkline_smoke TARGET qmltui_sparkline_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qmltui_add_scenario(NAME sparkline_demo TARGET qmltui_sparkline_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/sparkline.cast LABELS Cast)
endif()
