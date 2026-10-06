cmake_minimum_required(VERSION 3.20)

qt_add_executable(agent_stream_demo main.cpp)

qt_add_qml_module(agent_stream_demo
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
        FullScreenShell.qml
    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
        TARGET Qtui::Controls
        TARGET Qtui::TerminalHost
)

target_link_libraries(agent_stream_demo PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::TerminalHost)

if(TARGET Qtui::Testing)
    target_link_libraries(agent_stream_demo PRIVATE Qtui::Testing)
    include("${Qtui_SOURCE_DIR}/cmake/QtuiTesting.cmake")
    qtui_add_scenario_test(NAME agent_stream_modes TARGET agent_stream_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_modes.js"
        COLUMNS 120 ROWS 35 LABELS AgentSmoke)
endif()
