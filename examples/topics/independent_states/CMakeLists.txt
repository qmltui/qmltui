qt_add_executable(qmltui_independent_states_demo main.cpp)

qt_add_qml_module(qmltui_independent_states_demo
    URI IndependentStatesDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET
        qmltui::core
)

target_link_libraries(qmltui_independent_states_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_independent_states_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME independent_states_scenario TARGET qmltui_independent_states_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_independent_states.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
