cmake_minimum_required(VERSION 3.20)

set(agent_stream_qml_files
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
)

qt_add_executable(qamui_agent_stream_demo main.cpp)

qt_add_qml_module(qamui_agent_stream_demo
    URI AgentStreamDemo
    VERSION 1.0
    QML_FILES ${agent_stream_qml_files}
    DEPENDENCIES
        TARGET Qamui::Core
        TARGET Qamui::Controls
)

target_link_libraries(qamui_agent_stream_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_agent_stream_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME agent_stream_inline TARGET qamui_agent_stream_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_inline.js"
        COLUMNS 120 ROWS 35 LABELS AgentSmoke
        ARTIFACT "banner.txt" ARTIFACT_CONTENT "Qamui")
    qamui_add_scenario(NAME agent_stream_prompt_focus TARGET qamui_agent_stream_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_focus.js"
        COLUMNS 120 ROWS 35 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
