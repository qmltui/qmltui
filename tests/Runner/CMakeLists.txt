# Tests for Qamui::Runner: the fixture application and one scenario per
# runner contract (success, timeouts, artifacts, snapshots, environment modes).

add_qamui_test(test_runner_declarations test_runner_declarations.cpp)

# Consumer editor support: the helper materializes the declarations and a
# jsconfig into a project directory, and never clobbers an existing jsconfig.
set(qamui_editor_probe "${CMAKE_CURRENT_BINARY_DIR}/editor_support_fresh")
file(REMOVE_RECURSE "${qamui_editor_probe}")
file(MAKE_DIRECTORY "${qamui_editor_probe}")
qamui_scenario_editor_support(DIRECTORY "${qamui_editor_probe}")
add_test(NAME test_runner_editor_support
    COMMAND "${CMAKE_COMMAND}"
        "-DPROBE_DIR=${qamui_editor_probe}"
        "-DDECLARATIONS=${QAMUI_RUNNER_DECLARATIONS}"
        "-DEXPECT_GENERATED=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/editor_support_check.cmake")

set(qamui_editor_probe_seeded "${CMAKE_CURRENT_BINARY_DIR}/editor_support_seeded")
file(REMOVE_RECURSE "${qamui_editor_probe_seeded}")
file(MAKE_DIRECTORY "${qamui_editor_probe_seeded}")
file(WRITE "${qamui_editor_probe_seeded}/jsconfig.json" "{ \"pre-existing-marker\": true }\n")
qamui_scenario_editor_support(DIRECTORY "${qamui_editor_probe_seeded}")
add_test(NAME test_runner_editor_support_preserves_jsconfig
    COMMAND "${CMAKE_COMMAND}"
        "-DPROBE_DIR=${qamui_editor_probe_seeded}"
        "-DDECLARATIONS=${QAMUI_RUNNER_DECLARATIONS}"
        "-DEXPECT_PRESERVED=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/editor_support_check.cmake")
set_tests_properties(test_runner_editor_support
    test_runner_editor_support_preserves_jsconfig PROPERTIES LABELS "Runner")

add_executable(qamui_runner_fixture main.cpp)
target_link_libraries(qamui_runner_fixture PRIVATE
    Qt6::Core
    Qt6::Qml
    Qamui::Core
    Qamui::Runner
)
target_compile_definitions(qamui_runner_fixture PRIVATE
    QAMUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/Fixture.qml"
    QAMUI_RUNNER_INLINE_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/InlineFixture.qml"
)
qt_add_resources(qamui_runner_fixture embedded_runner_scenarios
    PREFIX "/qamui-tests"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/scenarios"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/resource_snapshot.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/resource_snapshot_stale.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/snapshots/fixture.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/snapshots/stale_seed.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/module_main.mjs"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/lib/module_helpers.mjs"
)

set(qamui_runner_scenarios
    "${CMAKE_CURRENT_SOURCE_DIR}/scenarios")

add_qamui_runner_scenario(test_runner_fixture_success qamui_runner_fixture
    "${qamui_runner_scenarios}/success.js" 0
    ARTIFACT "success.txt"
    ARTIFACT_CONTENT "INPUT: café 世界A"
)
add_qamui_runner_scenario(test_runner_fixture_timeout qamui_runner_fixture
    "${qamui_runner_scenarios}/timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_fixture_waits qamui_runner_fixture
    "${qamui_runner_scenarios}/waits.js" 0
    ARTIFACT "waits.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qamui_runner_scenario(test_runner_fixture_condition_timeout qamui_runner_fixture
    "${qamui_runner_scenarios}/condition_timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms waiting for condition"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_fixture_condition_error qamui_runner_fixture
    "${qamui_runner_scenarios}/condition_error.js" 1
    FAILURE_PATTERN "intentional condition failure"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_fixture_artifact_path qamui_runner_fixture
    "${qamui_runner_scenarios}/artifact_path.js" 1
    FAILURE_PATTERN "Artifact path must be a relative path"
    ARTIFACT "failure.txt"
    FORBIDDEN_ARTIFACT "${qamui_runner_artifacts}/escape.txt"
)
add_qamui_runner_scenario(test_runner_fixture_artifact_symlink qamui_runner_fixture
    "${qamui_runner_scenarios}/artifact_symlink.js" 1
    FAILURE_PATTERN "escapes the artifact directory"
    ARTIFACT "failure.txt"
    SYMLINK_TRAP "escape-link:${qamui_runner_artifacts}/symlink_escape_target"
    FORBIDDEN_ARTIFACT "${qamui_runner_artifacts}/symlink_escape_target/escaped.txt"
)
add_qamui_runner_scenario(test_runner_fixture_javascript_error qamui_runner_fixture
    "${qamui_runner_scenarios}/javascript_error.js" 1
    FAILURE_PATTERN "intentional JavaScript failure"
    ARTIFACT "failure.txt"
)
# expectRepaint() must be meaningful in both directions: it passes only when the
# application itself invalidated the scene, and fails when nothing changed.
add_qamui_runner_scenario(test_runner_repaint_scheduled qamui_runner_fixture
    "${qamui_runner_scenarios}/repaint_scheduled.js" 0
)
add_qamui_runner_scenario(test_runner_repaint_absent qamui_runner_fixture
    "${qamui_runner_scenarios}/repaint_absent.js" 1
    FAILURE_PATTERN "waiting for the application to schedule a repaint"
    ARTIFACT "failure.txt"
)
# Frame cost is exposed to scenarios and recorded in result.json.
add_qamui_runner_scenario(test_runner_frame_stats qamui_runner_fixture
    "${qamui_runner_scenarios}/frame_stats.js" 0
    ARTIFACT "result.json"
    ARTIFACT_CONTENT "paintMicrosAvg"
)

# A scenario that never yields must still produce a diagnosis rather than
# hanging until the process timeout kills it with no artifacts.
add_qamui_runner_scenario(test_runner_fixture_never_yields qamui_runner_fixture
    "${qamui_runner_scenarios}/never_yields.js" 1
    FAILURE_PATTERN "did not yield to the event loop"
    ARTIFACT "failure.txt"
    SCENARIO_TIMEOUT_MS 1500
)
add_qamui_runner_scenario(test_runner_failure_never_yields qamui_runner_fixture
    "${qamui_runner_scenarios}/failure_never_yields.js" 1
    FAILURE_PATTERN "Caught failure must still terminate"
    ARTIFACT "failure.txt"
    SCENARIO_TIMEOUT_MS 100
    PROCESS_TIMEOUT 5
)
add_qamui_runner_scenario(test_runner_fixture_missing_finish qamui_runner_fixture
    "${qamui_runner_scenarios}/missing_finish.js" 0
    ARTIFACT "missing_finish.txt"
    FORBIDDEN_ARTIFACT "${qamui_runner_artifacts}/failure.txt"
)
add_qamui_runner_scenario(test_runner_fixture_auto_finish qamui_runner_fixture
    "${qamui_runner_scenarios}/auto_finish.js" 0
    ARTIFACT "auto_finish.txt"
)
add_qamui_runner_scenario(test_runner_fixture_async_success qamui_runner_fixture
    "${qamui_runner_scenarios}/async_success.js" 0
    ARTIFACT "async_success.txt"
)
add_qamui_runner_scenario(test_runner_fixture_async_failure qamui_runner_fixture
    "${qamui_runner_scenarios}/async_failure.js" 1
    FAILURE_PATTERN "intentional async error"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_fixture_extended_features qamui_runner_fixture
    "${qamui_runner_scenarios}/extended_features.js" 0
    ARTIFACT "extended_features.txt"
)
add_qamui_runner_scenario(test_runner_selector_invalid_cell qamui_runner_fixture
    "${qamui_runner_scenarios}/selector_invalid_cell.js" 1
    FAILURE_PATTERN "Coordinates (-1,0) are outside the committed frame"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_selector_missing_text qamui_runner_fixture
    "${qamui_runner_scenarios}/selector_missing_text.js" 1
    FAILURE_PATTERN "was not found (0 matches)"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_snapshot_success qamui_runner_fixture
    "${qamui_runner_scenarios}/snapshot_success.js" 0
)
add_qamui_runner_scenario(test_runner_snapshot_resource qamui_runner_fixture
    "" 0
    EMBEDDED
)
# The embedded C++ API. The fixture drops the QAMUI_RUNNER_EMBEDDED_SCENARIO
# and QAMUI_RUNNER_LIVE hints the driver set, so the quiet run below rests on
# Runner::useEmbeddedScenario() alone and the live runs on its liveOutput
# argument or on setLiveOutput() after construction.
add_qamui_runner_scenario(test_runner_embedded_api qamui_runner_fixture
    "" 0
    EMBEDDED
    ARGUMENTS --embedded
)
add_qamui_runner_scenario(test_runner_embedded_api_live qamui_runner_fixture
    "" 0
    EMBEDDED LIVE
    ARGUMENTS --embedded --live
)
add_qamui_runner_scenario(test_runner_embedded_api_live_after_construction qamui_runner_fixture
    "" 0
    EMBEDDED LIVE
    ARGUMENTS --embedded --live-after-construction
)
# setUpdateSnapshots() wins over QAMUI_RUNNER_UPDATE_SNAPSHOTS in both
# directions, observed on a resource baseline that never matches: update mode
# fails with the resource diagnostic, normal mode with a plain mismatch.
add_qamui_runner_scenario(test_runner_embedded_api_update_snapshots qamui_runner_fixture
    "" 1
    EMBEDDED NO_UPDATE_SNAPSHOTS
    ARGUMENTS --embedded --update-snapshots
        "--scenario=:/qamui-tests/resource_snapshot_stale.js"
    FAILURE_PATTERN "cannot update resource snapshot"
    ARTIFACT "snapshots/stale_seed.actual.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qamui_runner_scenario(test_runner_embedded_api_no_update_snapshots qamui_runner_fixture
    "" 1
    EMBEDDED UPDATE_SNAPSHOTS
    ARGUMENTS --embedded --no-update-snapshots
        "--scenario=:/qamui-tests/resource_snapshot_stale.js"
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/stale_seed.diff"
)
add_qamui_runner_scenario(test_runner_snapshot_mismatch qamui_runner_fixture
    "${qamui_runner_scenarios}/snapshot_mismatch.js" 1
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/mismatch.diff"
    ARTIFACT_CONTENT "+++ actual"
)
add_qamui_runner_scenario(test_runner_styled_snapshot_success qamui_runner_fixture
    "${qamui_runner_scenarios}/styled_snapshot_success.js" 0
    ARTIFACT "styled.txt"
    ARTIFACT_CONTENT "--- styles"
)
add_qamui_runner_scenario(test_runner_styled_snapshot_mismatch qamui_runner_fixture
    "${qamui_runner_scenarios}/styled_snapshot_mismatch.js" 1
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/mismatch_styled.diff"
    ARTIFACT_CONTENT "fg=#7FDBCAFF"
)

# Snapshot update mode. The scenario runs from a build-tree copy: the record
# test seeds a stale baseline next to it, rewrites it under
# QAMUI_RUNNER_UPDATE_SNAPSHOTS=1, and the verify test then passes against the freshly
# recorded file without update mode.
set(qamui_runner_update_dir "${CMAKE_CURRENT_BINARY_DIR}/snapshot_update")
file(COPY "${qamui_runner_scenarios}/snapshot_update.js"
    DESTINATION "${qamui_runner_update_dir}")
add_qamui_runner_scenario(test_runner_snapshot_update_record qamui_runner_fixture
    "${qamui_runner_update_dir}/snapshot_update.js" 0
    UPDATE_SNAPSHOTS
    SNAPSHOT_SEED_SOURCE "${qamui_runner_scenarios}/snapshots/stale_seed.txt"
    SNAPSHOT_SEED_DEST "${qamui_runner_update_dir}/snapshots/updated_baseline.txt"
    EXPECT_UPDATED_SNAPSHOT "${qamui_runner_update_dir}/snapshots/updated_baseline.txt"
    EXPECT_UPDATED_SNAPSHOT_CONTENT "TESTRUNNER FIXTURE"
)
add_qamui_runner_scenario(test_runner_snapshot_update_verify qamui_runner_fixture
    "${qamui_runner_update_dir}/snapshot_update.js" 0
    NO_UPDATE_SNAPSHOTS
)
set_tests_properties(test_runner_snapshot_update_record PROPERTIES
    FIXTURES_SETUP qamui_snapshot_update)
set_tests_properties(test_runner_snapshot_update_verify PROPERTIES
    FIXTURES_REQUIRED qamui_snapshot_update)
# The virtual clock: deterministic timers, animations, and frame discipline.
add_qamui_runner_scenario(test_runner_virtual_clock qamui_runner_fixture
    "${qamui_runner_scenarios}/virtual_clock.js" 0
    CLOCK VIRTUAL
    ARTIFACT "virtual_clock.txt"
    ARTIFACT_CONTENT "TICKS: 5"
)
add_qamui_runner_scenario(test_runner_virtual_clock_hint qamui_runner_fixture
    "${qamui_runner_scenarios}/virtual_clock_hint.js" 1
    CLOCK VIRTUAL
    FAILURE_PATTERN "advance it with advanceTime()"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_advance_time_real_clock qamui_runner_fixture
    "${qamui_runner_scenarios}/advance_time_real.js" 1
    FAILURE_PATTERN "requires the virtual clock"
    ARTIFACT "failure.txt"
)

# Module scenarios and scope hygiene.
add_qamui_runner_scenario(test_runner_module_main qamui_runner_fixture
    "${qamui_runner_scenarios}/module_main.mjs" 0
    ARTIFACT "module_main.txt"
    ARTIFACT_CONTENT "INPUT: modules work"
)
# Modules also load from Qt resources, including their relative imports.
add_qamui_runner_scenario(test_runner_module_resource qamui_runner_fixture
    ":/qamui-tests/module_main.mjs" 0
    ARTIFACT "module_main.txt"
    ARTIFACT_CONTENT "INPUT: modules work"
)
add_qamui_runner_scenario(test_runner_module_no_main qamui_runner_fixture
    "${qamui_runner_scenarios}/module_no_main.mjs" 1
    FAILURE_PATTERN "Module scenario must export a function main(terminal)"
)
add_qamui_runner_scenario(test_runner_clean_globals qamui_runner_fixture
    "${qamui_runner_scenarios}/clean_globals.js" 0
)
add_qamui_runner_scenario(test_runner_error_line qamui_runner_fixture
    "${qamui_runner_scenarios}/error_line.js" 1
    FAILURE_PATTERN "error_line.js:7"
    ARTIFACT "failure.txt"
)

# Input completeness: hover gating, mouseDown/mouseUp, drag motion, paste,
# and single-grapheme text keys.
add_qamui_runner_scenario(test_runner_input_extended qamui_runner_fixture
    "${qamui_runner_scenarios}/input_extended.js" 0
    ARTIFACT "input_extended.txt"
    ARTIFACT_CONTENT "INPUT: line1 line2 tabé世"
)

add_qamui_runner_scenario(test_runner_clipboard qamui_runner_fixture
    "${qamui_runner_scenarios}/clipboard.js" 0
    ARTIFACT "clipboard.txt"
    ARTIFACT_CONTENT "CLIPBOARD: Unavailable"
)

# Options objects and scoped locators.
add_qamui_runner_scenario(test_runner_locators qamui_runner_fixture
    "${qamui_runner_scenarios}/locators.js" 0
)
add_qamui_runner_scenario(test_runner_options_unknown qamui_runner_fixture
    "${qamui_runner_scenarios}/options_unknown.js" 1
    FAILURE_PATTERN "Unknown option \"timout\" for waitForText()"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_item_missing qamui_runner_fixture
    "${qamui_runner_scenarios}/item_missing.js" 1
    FAILURE_PATTERN "No visible item with objectName"
    ARTIFACT "failure.txt"
)

# Runtime viewport changes.
add_qamui_runner_scenario(test_runner_resize qamui_runner_fixture
    "${qamui_runner_scenarios}/resize.js" 0
    ARTIFACT "resize.txt"
    ARTIFACT_CONTENT "SIZE: 120x40"
)
add_qamui_runner_scenario(test_runner_resize_invalid qamui_runner_fixture
    "${qamui_runner_scenarios}/resize_invalid.js" 1
    FAILURE_PATTERN "columns and rows between 1 and 1000"
    ARTIFACT "failure.txt"
)

# The watchdog accepts up to five minutes for a whole scenario; the
# per-operation limit stays at 30 s.
add_qamui_runner_scenario(test_runner_scenario_timeout_max qamui_runner_fixture
    "${qamui_runner_scenarios}/success.js" 0
    SCENARIO_TIMEOUT_MS 300000
    PROCESS_TIMEOUT 30
)
add_qamui_runner_scenario(test_runner_scenario_timeout_too_long qamui_runner_fixture
    "${qamui_runner_scenarios}/success.js" 1
    SCENARIO_TIMEOUT_MS 300001
    PROCESS_TIMEOUT 30
    FAILURE_PATTERN "between 1 and 300000"
)

add_qamui_runner_scenario(test_runner_snapshot_missing qamui_runner_fixture
    "${qamui_runner_scenarios}/snapshot_missing.js" 1
    NO_UPDATE_SNAPSHOTS
    FAILURE_PATTERN "QAMUI_RUNNER_UPDATE_SNAPSHOTS=1 to record it"
    ARTIFACT "failure.txt"
)
add_qamui_runner_scenario(test_runner_snapshot_update_resource qamui_runner_fixture
    "${qamui_runner_scenarios}/snapshot_update_resource.js" 1
    UPDATE_SNAPSHOTS
    FAILURE_PATTERN "cannot update resource snapshot"
    ARTIFACT "snapshots/_qamui-tests_resource_snapshot.actual.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)

add_qamui_environment_scenario(test_runner_capture qamui_runner_fixture 0
    CAPTURE_AFTER_MS 0
    ARTIFACT "capture.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
# The fixture flips its status 75 ms after start, so a 300 ms delay must observe
# the settled frame, and the styled capture must be written alongside the text.
add_qamui_environment_scenario(test_runner_capture_after qamui_runner_fixture 0
    CAPTURE_AFTER_MS 300
    ARTIFACT "capture_styled.txt"
    ARTIFACT_CONTENT "STATUS: ASYNC READY"
)
add_qamui_environment_scenario(test_runner_capture_too_long qamui_runner_fixture 1
    CAPTURE_AFTER_MS 9000
    FAILURE_PATTERN "must not exceed QAMUI_RUNNER_TIMEOUT_MS"
    ARTIFACT "result.json"
)
add_qamui_runner_scenario(test_runner_fixture_missing_main qamui_runner_fixture
    "${qamui_runner_scenarios}/missing_main.js" 1
    FAILURE_PATTERN "Scenario must define a function main(terminal)"
    ARTIFACT "failure.txt"
)
add_qamui_environment_scenario(test_runner_eval qamui_runner_fixture 0
    EVAL_CODE "terminal.captureText('eval.txt')"
    ARTIFACT "eval.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qamui_environment_scenario(test_runner_environment_conflict qamui_runner_fixture 1
    CONFLICT
    SCRIPT "${qamui_runner_scenarios}/success.js"
    EVAL_CODE "terminal.frameText()"
    FAILURE_PATTERN "mutually exclusive"
    ARTIFACT "result.json"
)

# Scrollback assertions: committed rows of an Inline-mode fixture are observed
# separately from the live frame.
add_qamui_runner_scenario(test_runner_scrollback_basic qamui_runner_fixture
    "${qamui_runner_scenarios}/scrollback_basic.js" 0
    ARTIFACT "scrollback.txt"
    ARTIFACT_CONTENT "hello scroll"
    ARGUMENTS --fixture=inline)
add_qamui_runner_scenario(test_runner_scrollback_from_row qamui_runner_fixture
    "${qamui_runner_scenarios}/scrollback_from_row.js" 0
    ARTIFACT "since.txt"
    ARTIFACT_CONTENT "MODEL ROW 1"
    ARGUMENTS --fixture=inline)
add_qamui_runner_scenario(test_runner_scrollback_snapshot qamui_runner_fixture
    "${qamui_runner_scenarios}/scrollback_snapshot.js" 0
    ARGUMENTS --fixture=inline)
add_qamui_runner_scenario(test_runner_scrollback_styled qamui_runner_fixture
    "${qamui_runner_scenarios}/scrollback_styled.js" 0
    ARTIFACT "styled.txt"
    ARTIFACT_CONTENT "bold"
    ARGUMENTS --fixture=inline)
add_qamui_runner_scenario(test_runner_scrollback_timeout qamui_runner_fixture
    "${qamui_runner_scenarios}/scrollback_timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms waiting for scrollback text"
    ARGUMENTS --fixture=inline)
add_qamui_runner_scenario(test_runner_scrollback_fullscreen qamui_runner_fixture
    "${qamui_runner_scenarios}/scrollback_fullscreen.js" 1
    FAILURE_PATTERN "needs a Terminal in Inline mode")

# Asciicast recording: the session bytes land in a cast under the artifacts.
add_qamui_runner_scenario(test_runner_record_basic qamui_runner_fixture
    "${qamui_runner_scenarios}/record_basic.js" 0
    CLOCK VIRTUAL
    RECORD fixture.cast
    ARTIFACT "fixture.cast"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE")
add_qamui_runner_scenario(test_runner_record_restart qamui_runner_fixture
    "${qamui_runner_scenarios}/record_restart.js" 0
    CLOCK VIRTUAL
    RECORD first.cast
    ARTIFACT "second.cast"
    ARTIFACT_CONTENT "two")
add_qamui_runner_scenario(test_runner_record_without_env qamui_runner_fixture
    "${qamui_runner_scenarios}/record_without_env.js" 1
    FAILURE_PATTERN "requires QAMUI_RUNNER_RECORD")
if(Python3_Interpreter_FOUND)
    add_test(NAME test_runner_record_basic_cast
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_cast.py"
                "${qamui_runner_artifacts}/test_runner_record_basic/fixture.cast"
                --marker start --marker typed --text "TESTRUNNER FIXTURE" --text cast --min-events 2 --resize 90x26 --min-duration 0.05)
    add_test(NAME test_runner_record_restart_cast
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_cast.py"
                "${qamui_runner_artifacts}/test_runner_record_restart/second.cast"
                --marker second --text "TESTRUNNER FIXTURE" --text two)
    set_tests_properties(test_runner_record_basic_cast PROPERTIES
        DEPENDS test_runner_record_basic LABELS "Runner;AgentSmoke")
    set_tests_properties(test_runner_record_restart_cast PROPERTIES
        DEPENDS test_runner_record_restart LABELS "Runner;AgentSmoke")
endif()
