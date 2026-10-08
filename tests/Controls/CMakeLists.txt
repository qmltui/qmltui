# Tests for Qtui::Controls.

add_qtui_test(test_popup test_popup.cpp)
add_qtui_test(test_indicators test_indicators.cpp)
add_qtui_test(test_standard_roles test_standard_roles.cpp)
add_qtui_test(test_terminal_colors test_terminal_colors.cpp)

# The colour fixture reuses the TestRunner fixture's main() with its own QML.
add_executable(qtui_terminal_colors_fixture ../TestRunner/main.cpp)
target_link_libraries(qtui_terminal_colors_fixture PRIVATE Qtui::Core Qtui::Controls Qtui::TestRunner)
target_compile_definitions(qtui_terminal_colors_fixture PRIVATE
    QTUI_TEST_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/TerminalColorsFixture.qml"
)
qtui_add_scenario_test(
    NAME test_terminal_colors_frames
    TARGET qtui_terminal_colors_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_terminal_colors_frames.js"
    COLUMNS 40
    ROWS 12
    LABELS AgentSmoke
)
