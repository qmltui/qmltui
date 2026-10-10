# Tests for Qtui::Runner: the fixture application and one scenario per
# runner contract (success, timeouts, artifacts, snapshots, environment modes).

add_qtui_test(test_runner_declarations test_runner_declarations.cpp)

add_executable(qtui_runner_fixture main.cpp)
target_link_libraries(qtui_runner_fixture PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Runner
)
target_compile_definitions(qtui_runner_fixture PRIVATE
    QTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/Fixture.qml"
)
qt_add_resources(qtui_runner_fixture embedded_runner_scenarios
    PREFIX "/qtui-tests"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/scenarios"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/resource_snapshot.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/snapshots/fixture.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/module_main.mjs"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/lib/module_helpers.mjs"
)

set(qtui_runner_scenarios
    "${CMAKE_CURRENT_SOURCE_DIR}/scenarios")

add_qtui_runner_scenario(test_runner_fixture_success qtui_runner_fixture
    "${qtui_runner_scenarios}/success.js" 0
    ARTIFACT "success.txt"
    ARTIFACT_CONTENT "INPUT: café 世界A"
)
add_qtui_runner_scenario(test_runner_fixture_timeout qtui_runner_fixture
    "${qtui_runner_scenarios}/timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_waits qtui_runner_fixture
    "${qtui_runner_scenarios}/waits.js" 0
    ARTIFACT "waits.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qtui_runner_scenario(test_runner_fixture_condition_timeout qtui_runner_fixture
    "${qtui_runner_scenarios}/condition_timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms waiting for condition"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_condition_error qtui_runner_fixture
    "${qtui_runner_scenarios}/condition_error.js" 1
    FAILURE_PATTERN "intentional condition failure"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_artifact_path qtui_runner_fixture
    "${qtui_runner_scenarios}/artifact_path.js" 1
    FAILURE_PATTERN "Artifact path must be a relative path"
    ARTIFACT "failure.txt"
    FORBIDDEN_ARTIFACT "${qtui_runner_artifacts}/escape.txt"
)
add_qtui_runner_scenario(test_runner_fixture_artifact_symlink qtui_runner_fixture
    "${qtui_runner_scenarios}/artifact_symlink.js" 1
    FAILURE_PATTERN "escapes the artifact directory"
    ARTIFACT "failure.txt"
    SYMLINK_TRAP "escape-link:${qtui_runner_artifacts}/symlink_escape_target"
    FORBIDDEN_ARTIFACT "${qtui_runner_artifacts}/symlink_escape_target/escaped.txt"
)
add_qtui_runner_scenario(test_runner_fixture_javascript_error qtui_runner_fixture
    "${qtui_runner_scenarios}/javascript_error.js" 1
    FAILURE_PATTERN "intentional JavaScript failure"
    ARTIFACT "failure.txt"
)
# expectRepaint() must be meaningful in both directions: it passes only when the
# application itself invalidated the scene, and fails when nothing changed.
add_qtui_runner_scenario(test_runner_repaint_scheduled qtui_runner_fixture
    "${qtui_runner_scenarios}/repaint_scheduled.js" 0
)
add_qtui_runner_scenario(test_runner_repaint_absent qtui_runner_fixture
    "${qtui_runner_scenarios}/repaint_absent.js" 1
    FAILURE_PATTERN "waiting for the application to schedule a repaint"
    ARTIFACT "failure.txt"
)
# Frame cost is exposed to scenarios and recorded in result.json.
add_qtui_runner_scenario(test_runner_frame_stats qtui_runner_fixture
    "${qtui_runner_scenarios}/frame_stats.js" 0
    ARTIFACT "result.json"
    ARTIFACT_CONTENT "paintMicrosAvg"
)

# A scenario that never yields must still produce a diagnosis rather than
# hanging until the process timeout kills it with no artifacts.
add_qtui_runner_scenario(test_runner_fixture_never_yields qtui_runner_fixture
    "${qtui_runner_scenarios}/never_yields.js" 1
    FAILURE_PATTERN "did not yield to the event loop"
    ARTIFACT "failure.txt"
    SCENARIO_TIMEOUT_MS 1500
)
add_qtui_runner_scenario(test_runner_fixture_missing_finish qtui_runner_fixture
    "${qtui_runner_scenarios}/missing_finish.js" 0
    ARTIFACT "missing_finish.txt"
    FORBIDDEN_ARTIFACT "${qtui_runner_artifacts}/failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_auto_finish qtui_runner_fixture
    "${qtui_runner_scenarios}/auto_finish.js" 0
    ARTIFACT "auto_finish.txt"
)
add_qtui_runner_scenario(test_runner_fixture_async_success qtui_runner_fixture
    "${qtui_runner_scenarios}/async_success.js" 0
    ARTIFACT "async_success.txt"
)
add_qtui_runner_scenario(test_runner_fixture_async_failure qtui_runner_fixture
    "${qtui_runner_scenarios}/async_failure.js" 1
    FAILURE_PATTERN "intentional async error"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_extended_features qtui_runner_fixture
    "${qtui_runner_scenarios}/extended_features.js" 0
    ARTIFACT "extended_features.txt"
)
add_qtui_runner_scenario(test_runner_selector_invalid_cell qtui_runner_fixture
    "${qtui_runner_scenarios}/selector_invalid_cell.js" 1
    FAILURE_PATTERN "Coordinates (-1,0) are outside the committed frame"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_selector_missing_text qtui_runner_fixture
    "${qtui_runner_scenarios}/selector_missing_text.js" 1
    FAILURE_PATTERN "was not found (0 matches)"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_snapshot_success qtui_runner_fixture
    "${qtui_runner_scenarios}/snapshot_success.js" 0
)
add_qtui_runner_scenario(test_runner_snapshot_resource qtui_runner_fixture
    "" 0
    EMBEDDED
)
add_qtui_runner_scenario(test_runner_snapshot_mismatch qtui_runner_fixture
    "${qtui_runner_scenarios}/snapshot_mismatch.js" 1
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/mismatch.diff"
    ARTIFACT_CONTENT "+++ actual"
)
add_qtui_runner_scenario(test_runner_styled_snapshot_success qtui_runner_fixture
    "${qtui_runner_scenarios}/styled_snapshot_success.js" 0
    ARTIFACT "styled.txt"
    ARTIFACT_CONTENT "--- styles"
)
add_qtui_runner_scenario(test_runner_styled_snapshot_mismatch qtui_runner_fixture
    "${qtui_runner_scenarios}/styled_snapshot_mismatch.js" 1
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/mismatch_styled.diff"
    ARTIFACT_CONTENT "fg=#7FDBCAFF"
)

# Snapshot update mode. The scenario runs from a build-tree copy: the record
# test seeds a stale baseline next to it, rewrites it under
# QTUI_UPDATE_SNAPSHOTS=1, and the verify test then passes against the freshly
# recorded file without update mode.
set(qtui_runner_update_dir "${CMAKE_CURRENT_BINARY_DIR}/snapshot_update")
file(COPY "${qtui_runner_scenarios}/snapshot_update.js"
    DESTINATION "${qtui_runner_update_dir}")
add_qtui_runner_scenario(test_runner_snapshot_update_record qtui_runner_fixture
    "${qtui_runner_update_dir}/snapshot_update.js" 0
    UPDATE_SNAPSHOTS
    SNAPSHOT_SEED_SOURCE "${qtui_runner_scenarios}/snapshots/stale_seed.txt"
    SNAPSHOT_SEED_DEST "${qtui_runner_update_dir}/snapshots/updated_baseline.txt"
    EXPECT_UPDATED_SNAPSHOT "${qtui_runner_update_dir}/snapshots/updated_baseline.txt"
    EXPECT_UPDATED_SNAPSHOT_CONTENT "TESTRUNNER FIXTURE"
)
add_qtui_runner_scenario(test_runner_snapshot_update_verify qtui_runner_fixture
    "${qtui_runner_update_dir}/snapshot_update.js" 0
    NO_UPDATE_SNAPSHOTS
)
set_tests_properties(test_runner_snapshot_update_record PROPERTIES
    FIXTURES_SETUP qtui_snapshot_update)
set_tests_properties(test_runner_snapshot_update_verify PROPERTIES
    FIXTURES_REQUIRED qtui_snapshot_update)
# The virtual clock: deterministic timers, animations, and frame discipline.
add_qtui_runner_scenario(test_runner_virtual_clock qtui_runner_fixture
    "${qtui_runner_scenarios}/virtual_clock.js" 0
    CLOCK VIRTUAL
    ARTIFACT "virtual_clock.txt"
    ARTIFACT_CONTENT "TICKS: 5"
)
add_qtui_runner_scenario(test_runner_virtual_clock_hint qtui_runner_fixture
    "${qtui_runner_scenarios}/virtual_clock_hint.js" 1
    CLOCK VIRTUAL
    FAILURE_PATTERN "advance it with advanceTime()"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_advance_time_real_clock qtui_runner_fixture
    "${qtui_runner_scenarios}/advance_time_real.js" 1
    FAILURE_PATTERN "requires the virtual clock"
    ARTIFACT "failure.txt"
)

# Module scenarios and scope hygiene.
add_qtui_runner_scenario(test_runner_module_main qtui_runner_fixture
    "${qtui_runner_scenarios}/module_main.mjs" 0
    ARTIFACT "module_main.txt"
    ARTIFACT_CONTENT "INPUT: modules work"
)
# Modules also load from Qt resources, including their relative imports.
add_qtui_runner_scenario(test_runner_module_resource qtui_runner_fixture
    ":/qtui-tests/module_main.mjs" 0
    ARTIFACT "module_main.txt"
    ARTIFACT_CONTENT "INPUT: modules work"
)
add_qtui_runner_scenario(test_runner_module_no_main qtui_runner_fixture
    "${qtui_runner_scenarios}/module_no_main.mjs" 1
    FAILURE_PATTERN "Module scenario must export a function main(terminal)"
)
add_qtui_runner_scenario(test_runner_clean_globals qtui_runner_fixture
    "${qtui_runner_scenarios}/clean_globals.js" 0
)
add_qtui_runner_scenario(test_runner_error_line qtui_runner_fixture
    "${qtui_runner_scenarios}/error_line.js" 1
    FAILURE_PATTERN "error_line.js:7"
    ARTIFACT "failure.txt"
)

# Input completeness: hover gating, mouseDown/mouseUp, drag motion, paste,
# and single-grapheme text keys.
add_qtui_runner_scenario(test_runner_input_extended qtui_runner_fixture
    "${qtui_runner_scenarios}/input_extended.js" 0
    ARTIFACT "input_extended.txt"
    ARTIFACT_CONTENT "INPUT: line1 line2 tabé世"
)

# Options objects and scoped locators.
add_qtui_runner_scenario(test_runner_locators qtui_runner_fixture
    "${qtui_runner_scenarios}/locators.js" 0
)
add_qtui_runner_scenario(test_runner_options_unknown qtui_runner_fixture
    "${qtui_runner_scenarios}/options_unknown.js" 1
    FAILURE_PATTERN "Unknown option \"timout\" for waitForText()"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_item_missing qtui_runner_fixture
    "${qtui_runner_scenarios}/item_missing.js" 1
    FAILURE_PATTERN "No visible item with objectName"
    ARTIFACT "failure.txt"
)

# Runtime viewport changes.
add_qtui_runner_scenario(test_runner_resize qtui_runner_fixture
    "${qtui_runner_scenarios}/resize.js" 0
    ARTIFACT "resize.txt"
    ARTIFACT_CONTENT "SIZE: 120x40"
)
add_qtui_runner_scenario(test_runner_resize_invalid qtui_runner_fixture
    "${qtui_runner_scenarios}/resize_invalid.js" 1
    FAILURE_PATTERN "columns and rows between 1 and 1000"
    ARTIFACT "failure.txt"
)

# The watchdog accepts up to five minutes for a whole scenario; the
# per-operation limit stays at 30 s.
add_qtui_runner_scenario(test_runner_scenario_timeout_max qtui_runner_fixture
    "${qtui_runner_scenarios}/success.js" 0
    SCENARIO_TIMEOUT_MS 300000
    PROCESS_TIMEOUT 30
)
add_qtui_runner_scenario(test_runner_scenario_timeout_too_long qtui_runner_fixture
    "${qtui_runner_scenarios}/success.js" 1
    SCENARIO_TIMEOUT_MS 300001
    PROCESS_TIMEOUT 30
    FAILURE_PATTERN "between 1 and 300000"
)

add_qtui_runner_scenario(test_runner_snapshot_missing qtui_runner_fixture
    "${qtui_runner_scenarios}/snapshot_missing.js" 1
    NO_UPDATE_SNAPSHOTS
    FAILURE_PATTERN "QTUI_UPDATE_SNAPSHOTS=1 to record it"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_snapshot_update_resource qtui_runner_fixture
    "${qtui_runner_scenarios}/snapshot_update_resource.js" 1
    UPDATE_SNAPSHOTS
    FAILURE_PATTERN "cannot update resource snapshot"
    ARTIFACT "snapshots/_qtui-tests_resource_snapshot.actual.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)

add_qtui_environment_scenario(test_runner_capture qtui_pos_cashier 0
    CAPTURE_AFTER_MS 0
    ARTIFACT "capture.txt"
    ARTIFACT_CONTENT "RapidPOS"
)
# The fixture flips its status 75 ms after start, so a 300 ms delay must observe
# the settled frame, and the styled capture must be written alongside the text.
add_qtui_environment_scenario(test_runner_capture_after qtui_runner_fixture 0
    CAPTURE_AFTER_MS 300
    ARTIFACT "capture_styled.txt"
    ARTIFACT_CONTENT "STATUS: ASYNC READY"
)
add_qtui_environment_scenario(test_runner_capture_too_long qtui_runner_fixture 1
    CAPTURE_AFTER_MS 9000
    FAILURE_PATTERN "must not exceed QTUI_RUNNER_TIMEOUT_MS"
    ARTIFACT "result.json"
)
add_qtui_runner_scenario(test_runner_fixture_missing_main qtui_runner_fixture
    "${qtui_runner_scenarios}/missing_main.js" 1
    FAILURE_PATTERN "Scenario must define a function main(terminal)"
    ARTIFACT "failure.txt"
)
add_qtui_environment_scenario(test_runner_eval qtui_runner_fixture 0
    EVAL_CODE "terminal.captureText('eval.txt')"
    ARTIFACT "eval.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qtui_environment_scenario(test_runner_environment_conflict qtui_runner_fixture 1
    CONFLICT
    SCRIPT "${qtui_runner_scenarios}/success.js"
    EVAL_CODE "terminal.frameText()"
    FAILURE_PATTERN "mutually exclusive"
    ARTIFACT "result.json"
)
