qt_add_executable(qmltui_animation_demo main.cpp)

qt_add_qml_module(qmltui_animation_demo
    URI AnimationDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET qmltui::core
)

target_link_libraries(qmltui_animation_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_animation_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME animation_smoke TARGET qmltui_animation_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
