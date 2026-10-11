qt_add_executable(qamui_tutorial_06_testing_with_runner main.cpp)
qt_add_qml_module(qamui_tutorial_06_testing_with_runner
    URI QamuiTutorial06 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
target_link_libraries(qamui_tutorial_06_testing_with_runner PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
if(TARGET Qamui::Runner)
    qt_add_executable(qamui_tutorial_06_testing_with_runner_test main.cpp)
    qt_add_qml_module(qamui_tutorial_06_testing_with_runner_test
        URI QamuiTutorial06 VERSION 1.0 QML_FILES Main.qml
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/QamuiTutorial06"
        DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
    target_link_libraries(qamui_tutorial_06_testing_with_runner_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
    qamui_add_scenario_test(NAME tutorial_06_smoke TARGET qamui_tutorial_06_testing_with_runner_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
    qamui_add_scenario(NAME tutorial_06_cast TARGET qamui_tutorial_06_testing_with_runner_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_record.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL RECORD tutorial.cast LABELS Tutorial)
endif()
