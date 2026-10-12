# Tests for qmltui::controls.

add_qmltui_test(test_popup test_popup.cpp)
add_qmltui_test(test_indicators test_indicators.cpp)
add_qmltui_test(test_standard_roles test_standard_roles.cpp)
add_qmltui_test(test_terminal_colors test_terminal_colors.cpp)
add_qmltui_test(test_controls_behaviour test_controls_behaviour.cpp)

# The colour fixture reuses the Runner fixture's main() with its own QML.
add_executable(qmltui_terminal_colors_fixture ../Runner/main.cpp)
target_link_libraries(qmltui_terminal_colors_fixture PRIVATE qmltui::core qmltui::controls qmltui::runner)
target_compile_definitions(qmltui_terminal_colors_fixture PRIVATE
    QMLTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/TerminalColorsFixture.qml"
)
qmltui_add_scenario(
    NAME test_terminal_colors_frames
    TARGET qmltui_terminal_colors_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_terminal_colors_frames.js"
    COLUMNS 40
    ROWS 12
    LABELS AgentSmoke
)

# Tooltip delay and timeout stepped on the virtual clock.
add_executable(qmltui_tooltip_clock_fixture ../Runner/main.cpp)
target_link_libraries(qmltui_tooltip_clock_fixture PRIVATE qmltui::core qmltui::controls qmltui::runner)
target_compile_definitions(qmltui_tooltip_clock_fixture PRIVATE
    QMLTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/ToolTipClockFixture.qml"
)
qmltui_add_scenario(
    NAME test_tooltip_clock
    TARGET qmltui_tooltip_clock_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tooltip_clock.js"
    COLUMNS 40
    ROWS 10
    CLOCK VIRTUAL
    LABELS AgentSmoke
)

# The default look of the button-like controls, as styled snapshots.
add_executable(qmltui_button_look_fixture ../Runner/main.cpp)
target_link_libraries(qmltui_button_look_fixture PRIVATE qmltui::core qmltui::controls qmltui::runner)
target_compile_definitions(qmltui_button_look_fixture PRIVATE
    QMLTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/ButtonLookFixture.qml"
)
qmltui_add_scenario(
    NAME test_button_look
    TARGET qmltui_button_look_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_button_look.js"
    COLUMNS 60
    ROWS 12
    LABELS AgentSmoke
)

# Popup frames scenario covering transparency, occlusion, nesting, resize, and dialogs.
add_executable(qmltui_popup_frames_fixture ../Runner/main.cpp)
target_link_libraries(qmltui_popup_frames_fixture PRIVATE qmltui::core qmltui::controls qmltui::runner)
target_compile_definitions(qmltui_popup_frames_fixture PRIVATE
    QMLTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/PopupFramesFixture.qml"
)
qmltui_add_scenario(
    NAME test_popup_frames
    TARGET qmltui_popup_frames_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_popup_frames.js"
    COLUMNS 50
    ROWS 16
    LABELS AgentSmoke
)
