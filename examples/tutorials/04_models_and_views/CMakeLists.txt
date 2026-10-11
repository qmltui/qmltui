qt_add_executable(qamui_tutorial_04_models_and_views main.cpp)
qt_add_qml_module(qamui_tutorial_04_models_and_views
    URI QamuiTutorial04 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
target_link_libraries(qamui_tutorial_04_models_and_views PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
if(TARGET Qamui::Runner)
    target_link_libraries(qamui_tutorial_04_models_and_views PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME tutorial_04_smoke TARGET qamui_tutorial_04_models_and_views
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
