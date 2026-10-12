# Tests for Qamui::Controls.

add_qamui_test(test_popup test_popup.cpp)
add_qamui_test(test_indicators test_indicators.cpp)
add_qamui_test(test_standard_roles test_standard_roles.cpp)
add_qamui_test(test_terminal_colors test_terminal_colors.cpp)
add_qamui_test(test_controls_behaviour test_controls_behaviour.cpp)

# The colour fixture reuses the Runner fixture's main() with its own QML.
add_executable(qamui_terminal_colors_fixture ../Runner/main.cpp)
target_link_libraries(qamui_terminal_colors_fixture PRIVATE Qamui::Core Qamui::Controls Qamui::Runner)
target_compile_definitions(qamui_terminal_colors_fixture PRIVATE
    QAMUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/TerminalColorsFixture.qml"
)
qamui_add_scenario(
    NAME test_terminal_colors_frames
    TARGET qamui_terminal_colors_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_terminal_colors_frames.js"
    COLUMNS 40
    ROWS 12
    LABELS AgentSmoke
)

# Tooltip delay and timeout stepped on the virtual clock.
add_executable(qamui_tooltip_clock_fixture ../Runner/main.cpp)
target_link_libraries(qamui_tooltip_clock_fixture PRIVATE Qamui::Core Qamui::Controls Qamui::Runner)
target_compile_definitions(qamui_tooltip_clock_fixture PRIVATE
    QAMUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/ToolTipClockFixture.qml"
)
qamui_add_scenario(
    NAME test_tooltip_clock
    TARGET qamui_tooltip_clock_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tooltip_clock.js"
    COLUMNS 40
    ROWS 10
    CLOCK VIRTUAL
    LABELS AgentSmoke
)

# The default look of the button-like controls, as styled snapshots.
add_executable(qamui_button_look_fixture ../Runner/main.cpp)
target_link_libraries(qamui_button_look_fixture PRIVATE Qamui::Core Qamui::Controls Qamui::Runner)
target_compile_definitions(qamui_button_look_fixture PRIVATE
    QAMUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/ButtonLookFixture.qml"
)
qamui_add_scenario(
    NAME test_button_look
    TARGET qamui_button_look_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_button_look.js"
    COLUMNS 60
    ROWS 12
    LABELS AgentSmoke
)

# Popup frames scenario covering transparency, occlusion, nesting, resize, and dialogs.
add_executable(qamui_popup_frames_fixture ../Runner/main.cpp)
target_link_libraries(qamui_popup_frames_fixture PRIVATE Qamui::Core Qamui::Controls Qamui::Runner)
target_compile_definitions(qamui_popup_frames_fixture PRIVATE
    QAMUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/PopupFramesFixture.qml"
)
qamui_add_scenario(
    NAME test_popup_frames
    TARGET qamui_popup_frames_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_popup_frames.js"
    COLUMNS 50
    ROWS 16
    LABELS AgentSmoke
)
