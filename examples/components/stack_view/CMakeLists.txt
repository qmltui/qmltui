qt_add_executable(qmltui_stack_view_demo main.cpp)
qt_add_qml_module(qmltui_stack_view_demo
    URI StackViewDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)
target_link_libraries(qmltui_stack_view_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_stack_view_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME stack_view_smoke TARGET qmltui_stack_view_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 100 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
    qmltui_add_scenario(NAME stack_view_demo TARGET qmltui_stack_view_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 100 ROWS 24 CLOCK VIRTUAL RECORD components/stack_view.cast LABELS Cast)
endif()
