qt_add_executable(qamui_stack_view_demo main.cpp)
qt_add_qml_module(qamui_stack_view_demo
    URI StackViewDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls
)
target_link_libraries(qamui_stack_view_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    qt_add_executable(qamui_stack_view_demo_test main.cpp)
    qt_add_qml_module(qamui_stack_view_demo_test
        URI StackViewDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/StackViewDemo"
        VERSION 1.0
        QML_FILES Main.qml
        DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls
    )
    target_link_libraries(qamui_stack_view_demo_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
    qamui_add_scenario(NAME stack_view_smoke TARGET qamui_stack_view_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 100 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
