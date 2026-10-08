# Tests for Qtui::TestRunner: the fixture application and one scenario per
# runner contract (success, timeouts, artifacts, snapshots, environment modes).

add_executable(qtui_test_runner_fixture main.cpp)
target_link_libraries(qtui_test_runner_fixture PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::TestRunner
)
target_compile_definitions(qtui_test_runner_fixture PRIVATE
    QTUI_TEST_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/Fixture.qml"
)
qt_add_resources(qtui_test_runner_fixture embedded_test_runner_scenarios
    PREFIX "/qtui-tests"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/scenarios"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/resource_snapshot.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/snapshots/fixture.txt"
)

set(qtui_test_runner_scenarios
    "${CMAKE_CURRENT_SOURCE_DIR}/scenarios")

add_qtui_runner_scenario(test_runner_fixture_success qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/success.js" 0
    ARTIFACT "success.txt"
    ARTIFACT_CONTENT "INPUT: café 世界A"
)
add_qtui_runner_scenario(test_runner_fixture_timeout qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_waits qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/waits.js" 0
    ARTIFACT "waits.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qtui_runner_scenario(test_runner_fixture_condition_timeout qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/condition_timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms waiting for condition"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_condition_error qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/condition_error.js" 1
    FAILURE_PATTERN "intentional condition failure"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_artifact_path qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/artifact_path.js" 1
    FAILURE_PATTERN "Artifact path must be a relative path"
    ARTIFACT "failure.txt"
    FORBIDDEN_ARTIFACT "${qtui_test_runner_artifacts}/escape.txt"
)
add_qtui_runner_scenario(test_runner_fixture_artifact_symlink qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/artifact_symlink.js" 1
    FAILURE_PATTERN "escapes the artifact directory"
    ARTIFACT "failure.txt"
    SYMLINK_TRAP "escape-link:${qtui_test_runner_artifacts}/symlink_escape_target"
    FORBIDDEN_ARTIFACT "${qtui_test_runner_artifacts}/symlink_escape_target/escaped.txt"
)
add_qtui_runner_scenario(test_runner_fixture_javascript_error qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/javascript_error.js" 1
    FAILURE_PATTERN "intentional JavaScript failure"
    ARTIFACT "failure.txt"
)
# expectRepaint() must be meaningful in both directions: it passes only when the
# application itself invalidated the scene, and fails when nothing changed.
add_qtui_runner_scenario(test_runner_repaint_scheduled qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/repaint_scheduled.js" 0
)
add_qtui_runner_scenario(test_runner_repaint_absent qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/repaint_absent.js" 1
    FAILURE_PATTERN "waiting for the application to schedule a repaint"
    ARTIFACT "failure.txt"
)
# Frame cost is exposed to scenarios and recorded in result.json.
add_qtui_runner_scenario(test_runner_frame_stats qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/frame_stats.js" 0
    ARTIFACT "result.json"
    ARTIFACT_CONTENT "paintMicrosAvg"
)

# A scenario that never yields must still produce a diagnosis rather than
# hanging until the process timeout kills it with no artifacts.
add_qtui_runner_scenario(test_runner_fixture_never_yields qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/never_yields.js" 1
    FAILURE_PATTERN "did not yield to the event loop"
    ARTIFACT "failure.txt"
    SCENARIO_TIMEOUT_MS 1500
)
add_qtui_runner_scenario(test_runner_fixture_missing_finish qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/missing_finish.js" 0
    ARTIFACT "missing_finish.txt"
    FORBIDDEN_ARTIFACT "${qtui_test_runner_artifacts}/failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_auto_finish qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/auto_finish.js" 0
    ARTIFACT "auto_finish.txt"
)
add_qtui_runner_scenario(test_runner_fixture_async_success qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/async_success.js" 0
    ARTIFACT "async_success.txt"
)
add_qtui_runner_scenario(test_runner_fixture_async_failure qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/async_failure.js" 1
    FAILURE_PATTERN "intentional async error"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_fixture_extended_features qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/extended_features.js" 0
    ARTIFACT "extended_features.txt"
)
add_qtui_runner_scenario(test_runner_selector_invalid_cell qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/selector_invalid_cell.js" 1
    FAILURE_PATTERN "Coordinates (-1,0) are outside the committed frame"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_selector_missing_text qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/selector_missing_text.js" 1
    FAILURE_PATTERN "was not found (0 matches)"
    ARTIFACT "failure.txt"
)
add_qtui_runner_scenario(test_runner_snapshot_success qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/snapshot_success.js" 0
)
add_qtui_runner_scenario(test_runner_snapshot_resource qtui_test_runner_fixture
    "" 0
    EMBEDDED
)
add_qtui_runner_scenario(test_runner_snapshot_mismatch qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/snapshot_mismatch.js" 1
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/mismatch.diff"
    ARTIFACT_CONTENT "+++ actual"
)
add_qtui_runner_scenario(test_runner_styled_snapshot_success qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/styled_snapshot_success.js" 0
    ARTIFACT "styled.txt"
    ARTIFACT_CONTENT "--- styles"
)
add_qtui_runner_scenario(test_runner_styled_snapshot_mismatch qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/styled_snapshot_mismatch.js" 1
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/mismatch_styled.diff"
    ARTIFACT_CONTENT "fg=#7FDBCAFF"
)

add_qtui_environment_scenario(test_runner_capture qtui_pos_cashier 0
    CAPTURE_AFTER_MS 0
    ARTIFACT "capture.txt"
    ARTIFACT_CONTENT "RapidPOS"
)
# The fixture flips its status 75 ms after start, so a 300 ms delay must observe
# the settled frame, and the styled capture must be written alongside the text.
add_qtui_environment_scenario(test_runner_capture_after qtui_test_runner_fixture 0
    CAPTURE_AFTER_MS 300
    ARTIFACT "capture_styled.txt"
    ARTIFACT_CONTENT "STATUS: ASYNC READY"
)
add_qtui_environment_scenario(test_runner_capture_too_long qtui_test_runner_fixture 1
    CAPTURE_AFTER_MS 9000
    FAILURE_PATTERN "must not exceed QTUI_TEST_TIMEOUT_MS"
    ARTIFACT "result.json"
)
add_qtui_runner_scenario(test_runner_fixture_missing_main qtui_test_runner_fixture
    "${qtui_test_runner_scenarios}/missing_main.js" 1
    FAILURE_PATTERN "Scenario must define a function main(terminal)"
    ARTIFACT "failure.txt"
)
add_qtui_environment_scenario(test_runner_eval qtui_test_runner_fixture 0
    EVAL_CODE "terminal.captureText('eval.txt')"
    ARTIFACT "eval.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qtui_environment_scenario(test_runner_environment_conflict qtui_test_runner_fixture 1
    CONFLICT
    SCRIPT "${qtui_test_runner_scenarios}/success.js"
    EVAL_CODE "terminal.frameText()"
    FAILURE_PATTERN "mutually exclusive"
    ARTIFACT "result.json"
)
