qt_add_executable(qamui_tab_bar_demo
    main.cpp
)

qt_add_qml_module(qamui_tab_bar_demo
    URI TabHeaderDemo
    VERSION 1.0
    QML_FILES
        Main.qml
 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_tab_bar_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_tab_bar_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME tab_bar_smoke TARGET qamui_tab_bar_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME tab_bar_demo TARGET qamui_tab_bar_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/tab_bar.cast LABELS Cast)
endif()
