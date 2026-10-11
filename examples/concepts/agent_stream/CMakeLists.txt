cmake_minimum_required(VERSION 3.20)

qt_add_executable(qamui_agent_stream_demo main.cpp)

qt_add_qml_module(qamui_agent_stream_demo
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
        TARGET Qamui::Core
        TARGET Qamui::Controls
)

target_link_libraries(qamui_agent_stream_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    qt_add_executable(qamui_agent_stream_demo_test main.cpp)
    target_link_libraries(qamui_agent_stream_demo_test PRIVATE
        Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
    target_compile_definitions(qamui_agent_stream_demo_test PRIVATE
        QAMUI_AGENT_STREAM_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Main.qml")
    qamui_add_scenario_test(NAME agent_stream_inline TARGET qamui_agent_stream_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_inline.js"
        COLUMNS 120 ROWS 35 LABELS AgentSmoke)
endif()
