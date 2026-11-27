# Tests for QmlTui::Runner: the fixture application and one scenario per
# runner contract (success, timeouts, artifacts, snapshots, environment modes).

add_qmltui_test(test_runner_declarations test_runner_declarations.cpp)

# Consumer editor support: the helper materializes the declarations and a
# jsconfig into a project directory, and never clobbers an existing jsconfig.
set(qmltui_editor_probe "${CMAKE_CURRENT_BINARY_DIR}/editor_support_fresh")
file(REMOVE_RECURSE "${qmltui_editor_probe}")
file(MAKE_DIRECTORY "${qmltui_editor_probe}")
qmltui_scenario_editor_support(DIRECTORY "${qmltui_editor_probe}")
add_test(NAME test_runner_editor_support
    COMMAND "${CMAKE_COMMAND}"
        "-DPROBE_DIR=${qmltui_editor_probe}"
        "-DDECLARATIONS=${QMLTUI_RUNNER_DECLARATIONS}"
        "-DEXPECT_GENERATED=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/editor_support_check.cmake")

set(qmltui_editor_probe_seeded "${CMAKE_CURRENT_BINARY_DIR}/editor_support_seeded")
file(REMOVE_RECURSE "${qmltui_editor_probe_seeded}")
file(MAKE_DIRECTORY "${qmltui_editor_probe_seeded}")
file(WRITE "${qmltui_editor_probe_seeded}/jsconfig.json" "{ \"pre-existing-marker\": true }\n")
qmltui_scenario_editor_support(DIRECTORY "${qmltui_editor_probe_seeded}")
add_test(NAME test_runner_editor_support_preserves_jsconfig
    COMMAND "${CMAKE_COMMAND}"
        "-DPROBE_DIR=${qmltui_editor_probe_seeded}"
        "-DDECLARATIONS=${QMLTUI_RUNNER_DECLARATIONS}"
        "-DEXPECT_PRESERVED=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/editor_support_check.cmake")
set(qmltui_editor_probe_tsconfig "${CMAKE_CURRENT_BINARY_DIR}/editor_support_tsconfig")
file(REMOVE_RECURSE "${qmltui_editor_probe_tsconfig}")
file(MAKE_DIRECTORY "${qmltui_editor_probe_tsconfig}")
file(WRITE "${qmltui_editor_probe_tsconfig}/tsconfig.json" "{ \"pre-existing-tsconfig\": true }\n")
qmltui_scenario_editor_support(DIRECTORY "${qmltui_editor_probe_tsconfig}")
add_test(NAME test_runner_editor_support_preserves_tsconfig
    COMMAND "${CMAKE_COMMAND}"
        "-DPROBE_DIR=${qmltui_editor_probe_tsconfig}"
        "-DDECLARATIONS=${QMLTUI_RUNNER_DECLARATIONS}"
        "-DEXPECT_TSCONFIG_PRESERVED=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/editor_support_check.cmake")
set_tests_properties(test_runner_editor_support
    test_runner_editor_support_preserves_jsconfig
    test_runner_editor_support_preserves_tsconfig PROPERTIES LABELS "Runner")

add_executable(qmltui_runner_fixture main.cpp)
target_link_libraries(qmltui_runner_fixture PRIVATE
    Qt6::Core
    Qt6::Qml
    QmlTui::Core
    QmlTui::Runner
)
target_compile_definitions(qmltui_runner_fixture PRIVATE
    QMLTUI_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/Fixture.qml"
    QMLTUI_RUNNER_INLINE_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/InlineFixture.qml"
)
qt_add_resources(qmltui_runner_fixture embedded_runner_scenarios
    PREFIX "/qmltui-tests"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/scenarios"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/resource_snapshot.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/resource_snapshot_stale.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/snapshots/fixture.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/snapshots/stale_seed.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/module_main.mjs"
        "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/lib/module_helpers.mjs"
)

set(qmltui_runner_artifacts
    "${CMAKE_CURRENT_BINARY_DIR}/RunnerArtifacts")
set(qmltui_runner_scenarios
    "${CMAKE_CURRENT_SOURCE_DIR}/scenarios")
unset(QMLTUI_RECORD_DIR)
unset(QMLTUI_RECORD_POST_COMMAND)

# A Runner self-test scenario driven by a script (or an embedded resource), with
# bounded timeouts and the artifact checks the runner contract promises.
function(add_qmltui_runner_scenario test_name executable script expected_exit)
    cmake_parse_arguments(SCENARIO "EMBEDDED;LIVE;UPDATE_SNAPSHOTS;NO_UPDATE_SNAPSHOTS"
        "FAILURE_PATTERN;ARTIFACT;ARTIFACT_CONTENT;FORBIDDEN_ARTIFACT;PROCESS_TIMEOUT;SYMLINK_TRAP;OPERATION_TIMEOUT_MS;SCENARIO_TIMEOUT_MS;SNAPSHOT_SEED_SOURCE;SNAPSHOT_SEED_DEST;EXPECT_UPDATED_SNAPSHOT;EXPECT_UPDATED_SNAPSHOT_CONTENT;CLOCK;RECORD"
        "ARGUMENTS;POST_COMMAND" ${ARGN})
    if(NOT SCENARIO_OPERATION_TIMEOUT_MS)
        set(SCENARIO_OPERATION_TIMEOUT_MS 1500)
    endif()
    if(NOT SCENARIO_SCENARIO_TIMEOUT_MS)
        set(SCENARIO_SCENARIO_TIMEOUT_MS 8000)
    endif()
    set(arguments
        NAME ${test_name}
        TARGET ${executable}
        EXPECTED_EXIT ${expected_exit}
        ARTIFACT_DIR "${qmltui_runner_artifacts}/${test_name}"
        OPERATION_TIMEOUT_MS ${SCENARIO_OPERATION_TIMEOUT_MS}
        SCENARIO_TIMEOUT_MS ${SCENARIO_SCENARIO_TIMEOUT_MS}
        LABELS Runner
    )
    if(SCENARIO_EMBEDDED)
        list(APPEND arguments EMBEDDED)
    else()
        list(APPEND arguments SCRIPT "${script}")
    endif()
    if(SCENARIO_LIVE)
        list(APPEND arguments LIVE)
    endif()
    if(SCENARIO_UPDATE_SNAPSHOTS)
        list(APPEND arguments UPDATE_SNAPSHOTS)
    endif()
    if(SCENARIO_NO_UPDATE_SNAPSHOTS)
        list(APPEND arguments NO_UPDATE_SNAPSHOTS)
    endif()
    if(SCENARIO_ARGUMENTS)
        list(APPEND arguments ARGUMENTS ${SCENARIO_ARGUMENTS})
    endif()
    if(SCENARIO_POST_COMMAND)
        list(APPEND arguments POST_COMMAND ${SCENARIO_POST_COMMAND})
    endif()
    foreach(argument FAILURE_PATTERN ARTIFACT ARTIFACT_CONTENT FORBIDDEN_ARTIFACT
            PROCESS_TIMEOUT SYMLINK_TRAP SNAPSHOT_SEED_SOURCE SNAPSHOT_SEED_DEST
            EXPECT_UPDATED_SNAPSHOT EXPECT_UPDATED_SNAPSHOT_CONTENT CLOCK RECORD)
        if(NOT "${SCENARIO_${argument}}" STREQUAL "")
            list(APPEND arguments ${argument} "${SCENARIO_${argument}}")
        endif()
    endforeach()
    qmltui_add_scenario(${arguments})
endfunction()

# A Runner session requested through the environment (QMLTUI_RUNNER_CAPTURE_AFTER_MS,
# QMLTUI_RUNNER_EVAL, or a deliberate conflict between modes).
function(add_qmltui_environment_scenario test_name executable expected_exit)
    cmake_parse_arguments(SCENARIO "CONFLICT"
        "CAPTURE_AFTER_MS;SCRIPT;EVAL_CODE;FAILURE_PATTERN;ARTIFACT;ARTIFACT_CONTENT" "" ${ARGN})
    set(command
        "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=$<TARGET_FILE:${executable}>"
        "-DARTIFACT_DIR=${qmltui_runner_artifacts}/${test_name}"
        "-DEXPECT_EXIT=${expected_exit}"
        "-DQML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
    )
    if(SCENARIO_CONFLICT)
        list(APPEND command
            "-DCONFLICT_SCENARIO=ON"
            "-DSCENARIO_SCRIPT=${SCENARIO_SCRIPT}"
            "-DEVAL_CODE=${SCENARIO_EVAL_CODE}"
        )
    elseif(DEFINED SCENARIO_CAPTURE_AFTER_MS)
        list(APPEND command "-DCAPTURE_AFTER_MS=${SCENARIO_CAPTURE_AFTER_MS}")
    elseif(DEFINED SCENARIO_EVAL_CODE)
        list(APPEND command "-DEVAL_CODE=${SCENARIO_EVAL_CODE}")
    else()
        message(FATAL_ERROR "Environment scenario requires CAPTURE_AFTER_MS or EVAL_CODE")
    endif()
    foreach(argument FAILURE_PATTERN ARTIFACT ARTIFACT_CONTENT)
        if(SCENARIO_${argument})
            list(APPEND command "-DEXPECT_${argument}=${SCENARIO_${argument}}")
        endif()
    endforeach()
    list(APPEND command "-P" "${QMLTUI_RUNNER_SCENARIO_DRIVER}")
    add_test(NAME ${test_name} COMMAND ${command})
    set_tests_properties(${test_name} PROPERTIES
        SKIP_RETURN_CODE 125
        TIMEOUT 15
        LABELS Runner
    )
endfunction()

add_qmltui_runner_scenario(test_runner_fixture_success qmltui_runner_fixture
    "${qmltui_runner_scenarios}/success.js" 0
    ARTIFACT "success.txt"
    ARTIFACT_CONTENT "INPUT: café 世界A"
)
add_qmltui_runner_scenario(test_runner_fixture_timeout qmltui_runner_fixture
    "${qmltui_runner_scenarios}/timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_waits qmltui_runner_fixture
    "${qmltui_runner_scenarios}/waits.js" 0
    ARTIFACT "waits.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qmltui_runner_scenario(test_runner_fixture_condition_timeout qmltui_runner_fixture
    "${qmltui_runner_scenarios}/condition_timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms waiting for condition"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_condition_error qmltui_runner_fixture
    "${qmltui_runner_scenarios}/condition_error.js" 1
    FAILURE_PATTERN "intentional condition failure"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_artifact_path qmltui_runner_fixture
    "${qmltui_runner_scenarios}/artifact_path.js" 1
    FAILURE_PATTERN "Artifact path must be a relative path"
    ARTIFACT "failure.txt"
    FORBIDDEN_ARTIFACT "${qmltui_runner_artifacts}/escape.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_artifact_symlink qmltui_runner_fixture
    "${qmltui_runner_scenarios}/artifact_symlink.js" 1
    FAILURE_PATTERN "escapes the artifact directory"
    ARTIFACT "failure.txt"
    SYMLINK_TRAP "escape-link:${qmltui_runner_artifacts}/symlink_escape_target"
    FORBIDDEN_ARTIFACT "${qmltui_runner_artifacts}/symlink_escape_target/escaped.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_javascript_error qmltui_runner_fixture
    "${qmltui_runner_scenarios}/javascript_error.js" 1
    FAILURE_PATTERN "intentional JavaScript failure"
    ARTIFACT "failure.txt"
)
# expectRepaint() must be meaningful in both directions: it passes only when the
# application itself invalidated the scene, and fails when nothing changed.
add_qmltui_runner_scenario(test_runner_repaint_scheduled qmltui_runner_fixture
    "${qmltui_runner_scenarios}/repaint_scheduled.js" 0
)
add_qmltui_runner_scenario(test_runner_repaint_absent qmltui_runner_fixture
    "${qmltui_runner_scenarios}/repaint_absent.js" 1
    FAILURE_PATTERN "waiting for the application to schedule a repaint"
    ARTIFACT "failure.txt"
)
# Frame cost is exposed to scenarios and recorded in result.json.
add_qmltui_runner_scenario(test_runner_frame_stats qmltui_runner_fixture
    "${qmltui_runner_scenarios}/frame_stats.js" 0
    ARTIFACT "result.json"
    ARTIFACT_CONTENT "paintMicrosAvg"
)

# A scenario that never yields must still produce a diagnosis rather than
# hanging until the process timeout kills it with no artifacts.
add_qmltui_runner_scenario(test_runner_fixture_never_yields qmltui_runner_fixture
    "${qmltui_runner_scenarios}/never_yields.js" 1
    FAILURE_PATTERN "did not yield to the event loop"
    ARTIFACT "failure.txt"
    SCENARIO_TIMEOUT_MS 1500
)
add_qmltui_runner_scenario(test_runner_failure_never_yields qmltui_runner_fixture
    "${qmltui_runner_scenarios}/failure_never_yields.js" 1
    FAILURE_PATTERN "Caught failure must still terminate"
    ARTIFACT "failure.txt"
    SCENARIO_TIMEOUT_MS 100
    PROCESS_TIMEOUT 5
)
add_qmltui_runner_scenario(test_runner_fixture_missing_finish qmltui_runner_fixture
    "${qmltui_runner_scenarios}/missing_finish.js" 0
    ARTIFACT "missing_finish.txt"
    FORBIDDEN_ARTIFACT "${qmltui_runner_artifacts}/failure.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_auto_finish qmltui_runner_fixture
    "${qmltui_runner_scenarios}/auto_finish.js" 0
    ARTIFACT "auto_finish.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_async_success qmltui_runner_fixture
    "${qmltui_runner_scenarios}/async_success.js" 0
    ARTIFACT "async_success.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_async_failure qmltui_runner_fixture
    "${qmltui_runner_scenarios}/async_failure.js" 1
    FAILURE_PATTERN "intentional async error"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_fixture_extended_features qmltui_runner_fixture
    "${qmltui_runner_scenarios}/extended_features.js" 0
    ARTIFACT "extended_features.txt"
)
add_qmltui_runner_scenario(test_runner_selector_invalid_cell qmltui_runner_fixture
    "${qmltui_runner_scenarios}/selector_invalid_cell.js" 1
    FAILURE_PATTERN "Coordinates (-1,0) are outside the committed frame"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_selector_missing_text qmltui_runner_fixture
    "${qmltui_runner_scenarios}/selector_missing_text.js" 1
    FAILURE_PATTERN "was not found (0 matches)"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_snapshot_success qmltui_runner_fixture
    "${qmltui_runner_scenarios}/snapshot_success.js" 0
)
add_qmltui_runner_scenario(test_runner_snapshot_resource qmltui_runner_fixture
    "" 0
    EMBEDDED
)
# The embedded C++ API. The fixture drops the QMLTUI_RUNNER_EMBEDDED_SCENARIO
# and QMLTUI_RUNNER_LIVE hints the driver set, so the quiet run below rests on
# Runner::useEmbeddedScenario() alone and the live runs on its liveOutput
# argument or on setLiveOutput() after construction.
add_qmltui_runner_scenario(test_runner_embedded_api qmltui_runner_fixture
    "" 0
    EMBEDDED
    ARGUMENTS --embedded
)
add_qmltui_runner_scenario(test_runner_embedded_api_live qmltui_runner_fixture
    "" 0
    EMBEDDED LIVE
    ARGUMENTS --embedded --live
)
add_qmltui_runner_scenario(test_runner_embedded_api_live_after_construction qmltui_runner_fixture
    "" 0
    EMBEDDED LIVE
    ARGUMENTS --embedded --live-after-construction
)
# setUpdateSnapshots() wins over QMLTUI_RUNNER_UPDATE_SNAPSHOTS in both
# directions, observed on a resource baseline that never matches: update mode
# fails with the resource diagnostic, normal mode with a plain mismatch.
add_qmltui_runner_scenario(test_runner_embedded_api_update_snapshots qmltui_runner_fixture
    "" 1
    EMBEDDED NO_UPDATE_SNAPSHOTS
    ARGUMENTS --embedded --update-snapshots
        "--scenario=:/qmltui-tests/resource_snapshot_stale.js"
    FAILURE_PATTERN "cannot update resource snapshot"
    ARTIFACT "snapshots/stale_seed.actual.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qmltui_runner_scenario(test_runner_embedded_api_no_update_snapshots qmltui_runner_fixture
    "" 1
    EMBEDDED UPDATE_SNAPSHOTS
    ARGUMENTS --embedded --no-update-snapshots
        "--scenario=:/qmltui-tests/resource_snapshot_stale.js"
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/stale_seed.diff"
)
add_qmltui_runner_scenario(test_runner_snapshot_mismatch qmltui_runner_fixture
    "${qmltui_runner_scenarios}/snapshot_mismatch.js" 1
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/mismatch.diff"
    ARTIFACT_CONTENT "+++ actual"
)
add_qmltui_runner_scenario(test_runner_styled_snapshot_success qmltui_runner_fixture
    "${qmltui_runner_scenarios}/styled_snapshot_success.js" 0
    ARTIFACT "styled.txt"
    ARTIFACT_CONTENT "--- styles"
)
add_qmltui_runner_scenario(test_runner_styled_snapshot_mismatch qmltui_runner_fixture
    "${qmltui_runner_scenarios}/styled_snapshot_mismatch.js" 1
    FAILURE_PATTERN "Snapshot mismatch"
    ARTIFACT "snapshots/mismatch_styled.diff"
    ARTIFACT_CONTENT "fg=#7FDBCAFF"
)

# Snapshot update mode. The scenario runs from a build-tree copy: the record
# test seeds a stale baseline next to it, rewrites it under
# QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1, and the verify test then passes against the freshly
# recorded file without update mode.
set(qmltui_runner_update_dir "${CMAKE_CURRENT_BINARY_DIR}/snapshot_update")
file(COPY "${qmltui_runner_scenarios}/snapshot_update.js"
    DESTINATION "${qmltui_runner_update_dir}")
add_qmltui_runner_scenario(test_runner_snapshot_update_record qmltui_runner_fixture
    "${qmltui_runner_update_dir}/snapshot_update.js" 0
    UPDATE_SNAPSHOTS
    SNAPSHOT_SEED_SOURCE "${qmltui_runner_scenarios}/snapshots/stale_seed.txt"
    SNAPSHOT_SEED_DEST "${qmltui_runner_update_dir}/snapshots/updated_baseline.txt"
    EXPECT_UPDATED_SNAPSHOT "${qmltui_runner_update_dir}/snapshots/updated_baseline.txt"
    EXPECT_UPDATED_SNAPSHOT_CONTENT "TESTRUNNER FIXTURE"
)
add_qmltui_runner_scenario(test_runner_snapshot_update_verify qmltui_runner_fixture
    "${qmltui_runner_update_dir}/snapshot_update.js" 0
    NO_UPDATE_SNAPSHOTS
)
set_tests_properties(test_runner_snapshot_update_record PROPERTIES
    FIXTURES_SETUP qmltui_snapshot_update)
set_tests_properties(test_runner_snapshot_update_verify PROPERTIES
    FIXTURES_REQUIRED qmltui_snapshot_update)
# The virtual clock: deterministic timers, animations, and frame discipline.
add_qmltui_runner_scenario(test_runner_virtual_clock qmltui_runner_fixture
    "${qmltui_runner_scenarios}/virtual_clock.js" 0
    CLOCK VIRTUAL
    ARTIFACT "virtual_clock.txt"
    ARTIFACT_CONTENT "TICKS: 5"
)
add_qmltui_runner_scenario(test_runner_virtual_clock_hint qmltui_runner_fixture
    "${qmltui_runner_scenarios}/virtual_clock_hint.js" 1
    CLOCK VIRTUAL
    FAILURE_PATTERN "advance it with advanceTime()"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_advance_time_real_clock qmltui_runner_fixture
    "${qmltui_runner_scenarios}/advance_time_real.js" 1
    FAILURE_PATTERN "requires the virtual clock"
    ARTIFACT "failure.txt"
)

# Module scenarios and scope hygiene.
add_qmltui_runner_scenario(test_runner_module_main qmltui_runner_fixture
    "${qmltui_runner_scenarios}/module_main.mjs" 0
    ARTIFACT "module_main.txt"
    ARTIFACT_CONTENT "INPUT: modules work"
)
# Modules also load from Qt resources, including their relative imports.
add_qmltui_runner_scenario(test_runner_module_resource qmltui_runner_fixture
    ":/qmltui-tests/module_main.mjs" 0
    ARTIFACT "module_main.txt"
    ARTIFACT_CONTENT "INPUT: modules work"
)
add_qmltui_runner_scenario(test_runner_module_no_main qmltui_runner_fixture
    "${qmltui_runner_scenarios}/module_no_main.mjs" 1
    FAILURE_PATTERN "Module scenario must export a function main(app)"
)
add_qmltui_runner_scenario(test_runner_clean_globals qmltui_runner_fixture
    "${qmltui_runner_scenarios}/clean_globals.js" 0
)
add_qmltui_runner_scenario(test_runner_error_line qmltui_runner_fixture
    "${qmltui_runner_scenarios}/error_line.js" 1
    FAILURE_PATTERN "error_line.js:7"
    ARTIFACT "failure.txt"
)

# Input completeness: hover gating, mouseDown/mouseUp, drag motion, paste,
# and single-grapheme text keys.
add_qmltui_runner_scenario(test_runner_input_extended qmltui_runner_fixture
    "${qmltui_runner_scenarios}/input_extended.js" 0
    ARTIFACT "input_extended.txt"
    ARTIFACT_CONTENT "INPUT: line1 line2 tabé世"
)

add_qmltui_runner_scenario(test_runner_clipboard qmltui_runner_fixture
    "${qmltui_runner_scenarios}/clipboard.js" 0
    ARTIFACT "clipboard.txt"
    ARTIFACT_CONTENT "CLIPBOARD: Unavailable"
)

# Options objects and scoped locators.
add_qmltui_runner_scenario(test_runner_locators qmltui_runner_fixture
    "${qmltui_runner_scenarios}/locators.js" 0
)
add_qmltui_runner_scenario(test_runner_options_unknown qmltui_runner_fixture
    "${qmltui_runner_scenarios}/options_unknown.js" 1
    FAILURE_PATTERN "Unknown option \"timout\" for waitForText()"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_item_missing qmltui_runner_fixture
    "${qmltui_runner_scenarios}/item_missing.js" 1
    FAILURE_PATTERN "No visible item with objectName"
    ARTIFACT "failure.txt"
)

# Runtime viewport changes.
add_qmltui_runner_scenario(test_runner_resize qmltui_runner_fixture
    "${qmltui_runner_scenarios}/resize.js" 0
    ARTIFACT "resize.txt"
    ARTIFACT_CONTENT "SIZE: 120x40"
)
add_qmltui_runner_scenario(test_runner_resize_invalid qmltui_runner_fixture
    "${qmltui_runner_scenarios}/resize_invalid.js" 1
    FAILURE_PATTERN "columns and rows between 1 and 1000"
    ARTIFACT "failure.txt"
)

# The watchdog accepts up to five minutes for a whole scenario; the
# per-operation limit stays at 30 s.
add_qmltui_runner_scenario(test_runner_scenario_timeout_max qmltui_runner_fixture
    "${qmltui_runner_scenarios}/success.js" 0
    SCENARIO_TIMEOUT_MS 300000
    PROCESS_TIMEOUT 30
)
add_qmltui_runner_scenario(test_runner_scenario_timeout_too_long qmltui_runner_fixture
    "${qmltui_runner_scenarios}/success.js" 1
    SCENARIO_TIMEOUT_MS 300001
    PROCESS_TIMEOUT 30
    FAILURE_PATTERN "between 1 and 300000"
)

add_qmltui_runner_scenario(test_runner_snapshot_missing qmltui_runner_fixture
    "${qmltui_runner_scenarios}/snapshot_missing.js" 1
    NO_UPDATE_SNAPSHOTS
    FAILURE_PATTERN "QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 to record it"
    ARTIFACT "failure.txt"
)
add_qmltui_runner_scenario(test_runner_snapshot_update_resource qmltui_runner_fixture
    "${qmltui_runner_scenarios}/snapshot_update_resource.js" 1
    UPDATE_SNAPSHOTS
    FAILURE_PATTERN "cannot update resource snapshot"
    ARTIFACT "snapshots/_qmltui-tests_resource_snapshot.actual.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)

add_qmltui_environment_scenario(test_runner_capture qmltui_runner_fixture 0
    CAPTURE_AFTER_MS 0
    ARTIFACT "capture.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
# The fixture flips its status 75 ms after start, so a 300 ms delay must observe
# the settled frame, and the styled capture must be written alongside the text.
add_qmltui_environment_scenario(test_runner_capture_after qmltui_runner_fixture 0
    CAPTURE_AFTER_MS 300
    ARTIFACT "capture_styled.txt"
    ARTIFACT_CONTENT "STATUS: ASYNC READY"
)
add_qmltui_environment_scenario(test_runner_capture_too_long qmltui_runner_fixture 1
    CAPTURE_AFTER_MS 29000
    FAILURE_PATTERN "must not exceed QMLTUI_RUNNER_TIMEOUT_MS"
    ARTIFACT "result.json"
)
add_qmltui_runner_scenario(test_runner_fixture_missing_main qmltui_runner_fixture
    "${qmltui_runner_scenarios}/missing_main.js" 1
    FAILURE_PATTERN "Scenario must define a function main(app)"
    ARTIFACT "failure.txt"
)
add_qmltui_environment_scenario(test_runner_eval qmltui_runner_fixture 0
    EVAL_CODE "app.captureText('eval.txt')"
    ARTIFACT "eval.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qmltui_environment_scenario(test_runner_environment_conflict qmltui_runner_fixture 1
    CONFLICT
    SCRIPT "${qmltui_runner_scenarios}/success.js"
    EVAL_CODE "app.frameText()"
    FAILURE_PATTERN "mutually exclusive"
    ARTIFACT "result.json"
)

# Scrollback assertions: committed rows of an Inline-mode fixture are observed
# separately from the live frame.
add_qmltui_runner_scenario(test_runner_scrollback_basic qmltui_runner_fixture
    "${qmltui_runner_scenarios}/scrollback_basic.js" 0
    ARTIFACT "scrollback.txt"
    ARTIFACT_CONTENT "hello scroll"
    ARGUMENTS --fixture=inline)
add_qmltui_runner_scenario(test_runner_scrollback_from_row qmltui_runner_fixture
    "${qmltui_runner_scenarios}/scrollback_from_row.js" 0
    ARTIFACT "since.txt"
    ARTIFACT_CONTENT "MODEL ROW 1"
    ARGUMENTS --fixture=inline)
add_qmltui_runner_scenario(test_runner_scrollback_snapshot qmltui_runner_fixture
    "${qmltui_runner_scenarios}/scrollback_snapshot.js" 0
    ARGUMENTS --fixture=inline)
add_qmltui_runner_scenario(test_runner_scrollback_styled qmltui_runner_fixture
    "${qmltui_runner_scenarios}/scrollback_styled.js" 0
    ARTIFACT "styled.txt"
    ARTIFACT_CONTENT "bold"
    ARGUMENTS --fixture=inline)
add_qmltui_runner_scenario(test_runner_scrollback_timeout qmltui_runner_fixture
    "${qmltui_runner_scenarios}/scrollback_timeout.js" 1
    FAILURE_PATTERN "Timed out after 100 ms waiting for scrollback text"
    ARGUMENTS --fixture=inline)
add_qmltui_runner_scenario(test_runner_scrollback_altscreen qmltui_runner_fixture
    "${qmltui_runner_scenarios}/scrollback_altscreen.js" 1
    FAILURE_PATTERN "needs a Terminal in Inline mode")
add_qmltui_runner_scenario(test_runner_inline_locators qmltui_runner_fixture
    "${qmltui_runner_scenarios}/inline_locators.js" 0
    ARGUMENTS --fixture=inline)
add_qmltui_runner_scenario(test_runner_item_tree qmltui_runner_fixture
    "${qmltui_runner_scenarios}/item_tree.js" 0)

# Asciicast recording: the session bytes land in a cast under the artifacts,
# and QMLTUI_RECORD_POST_COMMAND expands <CAST>, <CAST_STEM>, and <NAME>.
set(QMLTUI_RECORD_POST_COMMAND
    "${CMAKE_COMMAND}" -E copy "<CAST>" "<CAST_STEM>.<NAME>.cast")
add_qmltui_runner_scenario(test_runner_record_basic qmltui_runner_fixture
    "${qmltui_runner_scenarios}/record_basic.js" 0
    CLOCK VIRTUAL
    RECORD fixture.cast
    ARTIFACT "fixture.cast"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE")
unset(QMLTUI_RECORD_POST_COMMAND)
add_qmltui_runner_scenario(test_runner_record_restart qmltui_runner_fixture
    "${qmltui_runner_scenarios}/record_restart.js" 0
    CLOCK VIRTUAL
    RECORD first.cast
    ARTIFACT "second.cast"
    ARTIFACT_CONTENT "two")
add_qmltui_runner_scenario(test_runner_record_without_env qmltui_runner_fixture
    "${qmltui_runner_scenarios}/record_without_env.js" 1
    FAILURE_PATTERN "requires QMLTUI_RUNNER_RECORD")
if(Python3_Interpreter_FOUND)
    add_test(NAME test_runner_record_basic_cast
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_cast.py"
                "${qmltui_runner_artifacts}/test_runner_record_basic/fixture.test_runner_record_basic.cast"
                --marker start --marker typed --text "TESTRUNNER FIXTURE" --text cast --min-events 2 --resize 90x26 --min-duration 0.05)
    add_test(NAME test_runner_record_restart_cast
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_cast.py"
                "${qmltui_runner_artifacts}/test_runner_record_restart/second.cast"
                --marker second --text "TESTRUNNER FIXTURE" --text two)
    set_tests_properties(test_runner_record_basic_cast PROPERTIES
        DEPENDS test_runner_record_basic LABELS Runner)
    set_tests_properties(test_runner_record_restart_cast PROPERTIES
        DEPENDS test_runner_record_restart LABELS Runner)
endif()

add_qmltui_environment_scenario(test_runner_expect_style_unknown_property qmltui_runner_fixture 1
    EVAL_CODE "app.expectStyle(1, 1, { fg: '#ffffff' })"
    FAILURE_PATTERN "Unknown property \"fg\" for expectStyle()"
)
add_qmltui_environment_scenario(test_runner_expect_style_reverse_alias qmltui_runner_fixture 0
    EVAL_CODE "app.expectStyle(1, 1, { reverse: false })"
)
add_qmltui_environment_scenario(test_runner_expect_style_reverse_conflict qmltui_runner_fixture 1
    EVAL_CODE "app.expectStyle(1, 1, { reverse: true, reversed: false })"
    FAILURE_PATTERN "conflicting \"reverse\" and \"reversed\""
)

if(TARGET qmltui-runner)
    qt_add_executable(test_runner_launcher test_runner_launcher.cpp)
    target_link_libraries(test_runner_launcher PRIVATE Qt6::Core Qt6::Test)
    target_compile_definitions(test_runner_launcher PRIVATE
        QMLTUI_SCENARIOS="${CMAKE_CURRENT_SOURCE_DIR}/scenarios"
        QMLTUI_DEFAULT_TEST_LAUNCHER="$<TARGET_FILE:qmltui-runner>"
        QMLTUI_DEFAULT_TEST_FIXTURE="$<TARGET_FILE:qmltui_runner_fixture>"
        QMLTUI_DEFAULT_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}")
    add_dependencies(test_runner_launcher qmltui-runner qmltui_runner_fixture)
    add_test(NAME test_runner_launcher COMMAND test_runner_launcher)
    set_tests_properties(test_runner_launcher PROPERTIES
        LABELS "Runner;Unit"
        TIMEOUT 90
        ENVIRONMENT "QMLTUI_TEST_LAUNCHER=$<TARGET_FILE:qmltui-runner>;QMLTUI_TEST_FIXTURE=$<TARGET_FILE:qmltui_runner_fixture>;QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}")

    qt_add_executable(test_runner_session test_runner_session.cpp)
    target_link_libraries(test_runner_session PRIVATE Qt6::Core Qt6::Test)
    set(_qmltui_has_session_daemon 0)
    if(TARGET Qt6::Network)
        set(_qmltui_has_session_daemon 1)
    endif()
    target_compile_definitions(test_runner_session PRIVATE
        QMLTUI_RUNNER_HAS_SESSION_DAEMON=${_qmltui_has_session_daemon}
        QMLTUI_CMAKE_COMMAND="${CMAKE_COMMAND}"
        QMLTUI_DEFAULT_TEST_LAUNCHER="$<TARGET_FILE:qmltui-runner>"
        QMLTUI_DEFAULT_TEST_FIXTURE="$<TARGET_FILE:qmltui_runner_fixture>"
        QMLTUI_DEFAULT_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}")
    unset(_qmltui_has_session_daemon)
    add_dependencies(test_runner_session qmltui-runner qmltui_runner_fixture)
    add_test(NAME test_runner_session COMMAND test_runner_session)
    set_tests_properties(test_runner_session PROPERTIES
        LABELS "Runner;Unit"
        TIMEOUT 120
        ENVIRONMENT "QMLTUI_TEST_LAUNCHER=$<TARGET_FILE:qmltui-runner>;QMLTUI_TEST_FIXTURE=$<TARGET_FILE:qmltui_runner_fixture>;QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}")
endif()
