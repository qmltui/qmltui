qt_add_executable(qmltui_tutorial_05_controls_and_styling main.cpp)
qt_add_qml_module(qmltui_tutorial_05_controls_and_styling
    URI QmlTuiTutorial05 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls)
target_link_libraries(qmltui_tutorial_05_controls_and_styling PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
if(TARGET qmltui::runner)
    target_link_libraries(qmltui_tutorial_05_controls_and_styling PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME tutorial_05_smoke TARGET qmltui_tutorial_05_controls_and_styling
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
