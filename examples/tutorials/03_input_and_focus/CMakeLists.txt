qt_add_executable(qmltui_tutorial_03_input_and_focus main.cpp)
qt_add_qml_module(qmltui_tutorial_03_input_and_focus
    URI QmlTuiTutorial03 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls)
target_link_libraries(qmltui_tutorial_03_input_and_focus PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
if(TARGET qmltui::runner)
    target_link_libraries(qmltui_tutorial_03_input_and_focus PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME tutorial_03_smoke TARGET qmltui_tutorial_03_input_and_focus
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
