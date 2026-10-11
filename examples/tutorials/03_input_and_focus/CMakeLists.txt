qt_add_executable(qamui_tutorial_03_input_and_focus main.cpp)
qt_add_qml_module(qamui_tutorial_03_input_and_focus
    URI QamuiTutorial03 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
target_link_libraries(qamui_tutorial_03_input_and_focus PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
if(TARGET Qamui::Runner)
    qt_add_executable(qamui_tutorial_03_input_and_focus_test main.cpp)
    qt_add_qml_module(qamui_tutorial_03_input_and_focus_test
        URI QamuiTutorial03 VERSION 1.0 QML_FILES Main.qml
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/QamuiTutorial03"
        DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
    target_link_libraries(qamui_tutorial_03_input_and_focus_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
    qamui_add_scenario_test(NAME tutorial_03_smoke TARGET qamui_tutorial_03_input_and_focus_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
