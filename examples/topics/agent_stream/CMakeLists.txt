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

qt_add_executable(qmltui_agent_stream_demo main.cpp)

qt_add_qml_module(qmltui_agent_stream_demo
    URI AgentStreamDemo
    VERSION 1.0
    QML_FILES ${agent_stream_qml_files}
    DEPENDENCIES
        TARGET qmltui::core
        TARGET qmltui::controls
)

target_link_libraries(qmltui_agent_stream_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_agent_stream_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME agent_stream_inline TARGET qmltui_agent_stream_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_inline.js"
        COLUMNS 120 ROWS 35 LABELS AgentSmoke
        ARTIFACT "banner.txt" ARTIFACT_CONTENT "QmlTui")
    qmltui_add_scenario(NAME agent_stream_prompt_focus TARGET qmltui_agent_stream_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_focus.js"
        COLUMNS 120 ROWS 35 CLOCK VIRTUAL LABELS AgentSmoke)
    qmltui_add_scenario(NAME agent_stream_mouse TARGET qmltui_agent_stream_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_mouse.js"
        COLUMNS 120 ROWS 35 LABELS AgentSmoke)
endif()
