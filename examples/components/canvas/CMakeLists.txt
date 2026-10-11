qt_add_executable(qamui_canvas_demo main.cpp)

qt_add_qml_module(qamui_canvas_demo
    URI CanvasDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES
        TARGET Qamui::Core
)

target_link_libraries(qamui_canvas_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_canvas_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME canvas_smoke TARGET qamui_canvas_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME canvas_demo TARGET qamui_canvas_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/canvas.cast LABELS Cast)
endif()
