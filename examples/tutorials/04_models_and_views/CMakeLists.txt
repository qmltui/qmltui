qt_add_executable(qamui_tutorial_04_models_and_views main.cpp)
qt_add_qml_module(qamui_tutorial_04_models_and_views
    URI QamuiTutorial04 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
target_link_libraries(qamui_tutorial_04_models_and_views PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
if(TARGET Qamui::Runner)
    qt_add_executable(qamui_tutorial_04_models_and_views_test main.cpp)
    qt_add_qml_module(qamui_tutorial_04_models_and_views_test
        URI QamuiTutorial04 VERSION 1.0 QML_FILES Main.qml
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/QamuiTutorial04"
        DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
    target_link_libraries(qamui_tutorial_04_models_and_views_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
    qamui_add_scenario_test(NAME tutorial_04_smoke TARGET qamui_tutorial_04_models_and_views_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
