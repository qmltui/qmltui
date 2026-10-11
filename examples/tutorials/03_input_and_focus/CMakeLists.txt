qt_add_executable(qamui_tutorial_03_input_and_focus main.cpp)
qt_add_qml_module(qamui_tutorial_03_input_and_focus
    URI QamuiTutorial03 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
target_link_libraries(qamui_tutorial_03_input_and_focus PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
if(TARGET Qamui::Runner)
    target_link_libraries(qamui_tutorial_03_input_and_focus PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME tutorial_03_smoke TARGET qamui_tutorial_03_input_and_focus
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
