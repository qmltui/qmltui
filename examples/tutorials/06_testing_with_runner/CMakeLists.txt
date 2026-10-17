qt_add_executable(qmltui_tutorial_06 main.cpp)
set_target_properties(qmltui_tutorial_06 PROPERTIES OUTPUT_NAME qmltui_tutorial_06_testing_with_runner)
qt_add_qml_module(qmltui_tutorial_06
    URI QmlTuiTutorial06 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls)
target_link_libraries(qmltui_tutorial_06 PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
if(TARGET qmltui::runner)
    target_link_libraries(qmltui_tutorial_06 PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME tutorial_06_smoke TARGET qmltui_tutorial_06
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
    qmltui_add_scenario(NAME tutorial_06_cast TARGET qmltui_tutorial_06
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_record.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL RECORD tutorial.cast LABELS Tutorial)
endif()
add_executable(qmltui_tutorial_06_testing_with_runner ALIAS qmltui_tutorial_06)
