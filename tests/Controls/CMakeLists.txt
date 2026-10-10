# Tests for Qtui::Controls.

add_qtui_test(test_popup test_popup.cpp)
add_qtui_test(test_indicators test_indicators.cpp)
add_qtui_test(test_standard_roles test_standard_roles.cpp)
add_qtui_test(test_terminal_colors test_terminal_colors.cpp)
add_qtui_test(test_controls_behaviour test_controls_behaviour.cpp)

# The colour fixture reuses the Runner fixture's main() with its own QML.
add_executable(qtui_terminal_colors_fixture ../Runner/main.cpp)
target_link_libraries(qtui_terminal_colors_fixture PRIVATE Qtui::Core Qtui::Controls Qtui::Runner)
target_compile_definitions(qtui_terminal_colors_fixture PRIVATE
    QTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/TerminalColorsFixture.qml"
)
qtui_add_scenario(
    NAME test_terminal_colors_frames
    TARGET qtui_terminal_colors_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_terminal_colors_frames.js"
    COLUMNS 40
    ROWS 12
    LABELS AgentSmoke
)

# Tooltip delay and timeout stepped on the virtual clock.
add_executable(qtui_tooltip_clock_fixture ../Runner/main.cpp)
target_link_libraries(qtui_tooltip_clock_fixture PRIVATE Qtui::Core Qtui::Controls Qtui::Runner)
target_compile_definitions(qtui_tooltip_clock_fixture PRIVATE
    QTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/ToolTipClockFixture.qml"
)
qtui_add_scenario(
    NAME test_tooltip_clock
    TARGET qtui_tooltip_clock_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tooltip_clock.js"
    COLUMNS 40
    ROWS 10
    CLOCK VIRTUAL
    LABELS AgentSmoke
)

# The default look of the button-like controls, as styled snapshots.
add_executable(qtui_button_look_fixture ../Runner/main.cpp)
target_link_libraries(qtui_button_look_fixture PRIVATE Qtui::Core Qtui::Controls Qtui::Runner)
target_compile_definitions(qtui_button_look_fixture PRIVATE
    QTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/ButtonLookFixture.qml"
)
qtui_add_scenario(
    NAME test_button_look
    TARGET qtui_button_look_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_button_look.js"
    COLUMNS 60
    ROWS 12
    LABELS AgentSmoke
)
