qt_add_executable(qamui_mouse_interaction_demo main.cpp)

qt_add_qml_module(qamui_mouse_interaction_demo
    URI MouseInteractions
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_mouse_interaction_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_mouse_interaction_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME mouse_interaction_smoke TARGET qamui_mouse_interaction_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
