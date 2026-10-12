qt_add_executable(qmltui_tree_view_demo main.cpp)

qt_add_qml_module(qmltui_tree_view_demo
    URI TreeViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml
 DEPENDENCIES TARGET qmltui::core qmltui::controls
)

target_link_libraries(qmltui_tree_view_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_tree_view_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME tree_view_smoke TARGET qmltui_tree_view_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qmltui_add_scenario(NAME tree_view_demo TARGET qmltui_tree_view_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/tree_view.cast LABELS Cast)
endif()
