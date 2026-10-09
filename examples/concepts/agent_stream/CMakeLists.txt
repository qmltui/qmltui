cmake_minimum_required(VERSION 3.20)

qt_add_executable(qtui_agent_stream_demo main.cpp)

qt_add_qml_module(qtui_agent_stream_demo
    URI AgentStreamDemo
    VERSION 1.0
    QML_FILES
        StatusBadge.qml
        Main.qml
        Theme.qml
        BannerCard.qml
        UserPromptCard.qml
        ThoughtCard.qml
        ToolCallCard.qml
        ToolResultCard.qml
        DiffCard.qml
        PermissionCard.qml
        AgentResponseCard.qml
        SystemMessageCard.qml
        ErrorMessageCard.qml
        PinnedPromptBar.qml
        SwarmTableCard.qml
        RepoTreeCard.qml
        CommandPalette.qml
        ModelPickerModal.qml
        DiffInspectorModal.qml
        ToastNotification.qml
    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
)

target_link_libraries(qtui_agent_stream_demo PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

if(TARGET Qtui::TestRunner)
    qt_add_executable(qtui_agent_stream_demo_test main.cpp)
    target_link_libraries(qtui_agent_stream_demo_test PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::TestRunner)
    target_compile_definitions(qtui_agent_stream_demo_test PRIVATE
        QTUI_AGENT_STREAM_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Main.qml")
    include("${Qtui_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
    qtui_add_scenario_test(NAME agent_stream_inline TARGET qtui_agent_stream_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_inline.js"
        COLUMNS 120 ROWS 35 LABELS AgentSmoke)
endif()
