qt_add_executable(qmltui_tutorial_02_layout_and_text main.cpp)
qt_add_qml_module(qmltui_tutorial_02_layout_and_text
    URI QmlTuiTutorial02 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls)
target_link_libraries(qmltui_tutorial_02_layout_and_text PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
if(TARGET qmltui::runner)
    target_link_libraries(qmltui_tutorial_02_layout_and_text PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME tutorial_02_smoke TARGET qmltui_tutorial_02_layout_and_text
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
