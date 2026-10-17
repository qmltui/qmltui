qt_add_executable(qmltui_tutorial_05 main.cpp)
set_target_properties(qmltui_tutorial_05 PROPERTIES OUTPUT_NAME qmltui_tutorial_05_controls_and_styling)
qt_add_qml_module(qmltui_tutorial_05
    URI QmlTuiTutorial05 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls)
target_link_libraries(qmltui_tutorial_05 PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
if(TARGET qmltui::runner)
    target_link_libraries(qmltui_tutorial_05 PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME tutorial_05_smoke TARGET qmltui_tutorial_05
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
add_executable(qmltui_tutorial_05_controls_and_styling ALIAS qmltui_tutorial_05)
