foreach(required TEST_EXECUTABLE ARTIFACT_DIR EXPECT_EXIT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing required argument: ${required}")
    endif()
endforeach()

set(mode_count 0)
if(CONFLICT_SCENARIO)
    if(NOT SCENARIO_SCRIPT OR NOT EVAL_CODE)
        message(FATAL_ERROR "CONFLICT_SCENARIO requires SCENARIO_SCRIPT and EVAL_CODE")
    endif()
    set(mode_count 1)
else()
    foreach(mode EMBEDDED_SCENARIO)
        if(${mode})
            math(EXPR mode_count "${mode_count} + 1")
        endif()
    endforeach()
    foreach(value SCENARIO_SCRIPT EVAL_CODE CAPTURE_AFTER_MS)
        if(DEFINED ${value} AND NOT "${${value}}" STREQUAL "")
            math(EXPR mode_count "${mode_count} + 1")
        endif()
    endforeach()
endif()
if(NOT mode_count EQUAL 1)
    message(FATAL_ERROR "Exactly one scenario mode is required")
endif()

set(scenario_environment
    # Windows routes qWarning() to the debugger when no console is attached;
    # the failure-pattern checks below need it on stderr.
    "QT_FORCE_STDERR_LOGGING=1"
    "--unset=QMLTUI_RUNNER_SCRIPT"
    "--unset=QMLTUI_RUNNER_EVAL"
    "--unset=QMLTUI_RUNNER_CAPTURE_AFTER_MS"
    "--unset=QMLTUI_RUNNER_EMBEDDED_SCENARIO"
    "--unset=QMLTUI_RUNNER_RECORD"
)
if(CONFLICT_SCENARIO)
    list(APPEND scenario_environment
        "QMLTUI_RUNNER_SCRIPT=${SCENARIO_SCRIPT}"
        "QMLTUI_RUNNER_EVAL=${EVAL_CODE}"
    )
elseif(EMBEDDED_SCENARIO)
    list(APPEND scenario_environment "QMLTUI_RUNNER_EMBEDDED_SCENARIO=1")
elseif(DEFINED CAPTURE_AFTER_MS AND NOT CAPTURE_AFTER_MS STREQUAL "")
    list(APPEND scenario_environment "QMLTUI_RUNNER_CAPTURE_AFTER_MS=${CAPTURE_AFTER_MS}")
elseif(DEFINED EVAL_CODE AND NOT EVAL_CODE STREQUAL "")
    list(APPEND scenario_environment "QMLTUI_RUNNER_EVAL=${EVAL_CODE}")
else()
    list(APPEND scenario_environment "QMLTUI_RUNNER_SCRIPT=${SCENARIO_SCRIPT}")
endif()

if(NOT DEFINED COLUMNS)
    set(COLUMNS 80)
endif()
if(NOT DEFINED ROWS)
    set(ROWS 24)
endif()
if(NOT DEFINED OPERATION_TIMEOUT_MS)
    set(OPERATION_TIMEOUT_MS 2000)
endif()
if(NOT DEFINED SCENARIO_TIMEOUT_MS)
    set(SCENARIO_TIMEOUT_MS 30000)
endif()
if(NOT DEFINED PROCESS_TIMEOUT)
    math(EXPR PROCESS_TIMEOUT "(${SCENARIO_TIMEOUT_MS} + 999) / 1000 + 5")
endif()
if(LIVE_SCENARIO)
    set(live_value 1)
else()
    set(live_value 0)
endif()

if(DEFINED CLOCK AND NOT "${CLOCK}" STREQUAL "")
    list(APPEND scenario_environment "QMLTUI_RUNNER_CLOCK=${CLOCK}")
else()
    # A globally exported QMLTUI_RUNNER_CLOCK must not silently flip scenarios
    # that were written for the real clock.
    list(APPEND scenario_environment "--unset=QMLTUI_RUNNER_CLOCK")
endif()

# RECORD routes the terminal bytes into an asciicast under the artifact directory.
if(DEFINED RECORD AND NOT "${RECORD}" STREQUAL "")
    list(APPEND scenario_environment "QMLTUI_RUNNER_RECORD=${ARTIFACT_DIR}/${RECORD}")
endif()

# Snapshot update mode: force it on or off for this test; otherwise the
# executable inherits QMLTUI_RUNNER_UPDATE_SNAPSHOTS from the caller's environment.
if(UPDATE_SNAPSHOTS)
    list(APPEND scenario_environment "QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1")
elseif(NO_UPDATE_SNAPSHOTS)
    list(APPEND scenario_environment "--unset=QMLTUI_RUNNER_UPDATE_SNAPSHOTS")
endif()

# A seeded baseline makes the update path deterministic on every run: the
# stale seed is copied over the recorded baseline before the process starts.
if(DEFINED SNAPSHOT_SEED_SOURCE AND NOT "${SNAPSHOT_SEED_SOURCE}" STREQUAL "")
    get_filename_component(seed_parent "${SNAPSHOT_SEED_DEST}" DIRECTORY)
    file(MAKE_DIRECTORY "${seed_parent}")
    file(COPY_FILE "${SNAPSHOT_SEED_SOURCE}" "${SNAPSHOT_SEED_DEST}")
endif()

file(REMOVE_RECURSE "${ARTIFACT_DIR}")
file(MAKE_DIRECTORY "${ARTIFACT_DIR}")
if(DEFINED FORBIDDEN_ARTIFACT)
    file(REMOVE "${FORBIDDEN_ARTIFACT}")
endif()

# SYMLINK_TRAP=<name>:<target> plants a symlink inside the artifact directory so
# a scenario can verify that artifact containment resolves symlinks instead of
# only checking the path text.
if(DEFINED SYMLINK_TRAP AND NOT "${SYMLINK_TRAP}" STREQUAL "")
    # The name never contains a colon; the target may (Windows drive letters).
    string(FIND "${SYMLINK_TRAP}" ":" trap_separator)
    if(trap_separator LESS 0)
        message(FATAL_ERROR "SYMLINK_TRAP must be <name>:<target>, got '${SYMLINK_TRAP}'")
    endif()
    string(SUBSTRING "${SYMLINK_TRAP}" 0 ${trap_separator} trap_name)
    math(EXPR trap_target_start "${trap_separator} + 1")
    string(SUBSTRING "${SYMLINK_TRAP}" ${trap_target_start} -1 trap_target)
    file(MAKE_DIRECTORY "${trap_target}")
    file(CREATE_LINK "${trap_target}" "${ARTIFACT_DIR}/${trap_name}" SYMBOLIC RESULT trap_result)
    if(NOT trap_result EQUAL 0)
        # Windows needs Developer Mode or elevation to create symlinks. Report a
        # skip (SKIP_RETURN_CODE 125) where CMake can, otherwise fail loudly.
        if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.29)
            message(STATUS "Skipping: cannot create symlink trap on this host: ${trap_result}")
            cmake_language(EXIT 125)
        endif()
        message(FATAL_ERROR "Cannot create symlink trap: ${trap_result}")
    endif()
endif()

# An empty QML_IMPORT_PATH would blank out a value the caller exported, so only
# pass the import path through when we actually have one.
set(import_path_environment)
if(NOT "${QML_IMPORT_PATH}" STREQUAL "")
    list(APPEND import_path_environment
        "QML2_IMPORT_PATH=${QML_IMPORT_PATH}"
        "QML_IMPORT_PATH=${QML_IMPORT_PATH}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        ${scenario_environment}
        "QMLTUI_RUNNER_ARTIFACT_DIR=${ARTIFACT_DIR}"
        "QMLTUI_RUNNER_COLUMNS=${COLUMNS}"
        "QMLTUI_RUNNER_ROWS=${ROWS}"
        "QMLTUI_RUNNER_TIMEOUT_MS=${SCENARIO_TIMEOUT_MS}"
        "QMLTUI_RUNNER_OPERATION_TIMEOUT_MS=${OPERATION_TIMEOUT_MS}"
        "QMLTUI_RUNNER_LIVE=${live_value}"
        ${import_path_environment}
        "${TEST_EXECUTABLE}" ${TEST_ARGUMENTS}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT ${PROCESS_TIMEOUT}
)

set(output "${stdout}\n${stderr}")
if(LIVE_SCENARIO AND stdout STREQUAL "")
    message(FATAL_ERROR "Live scenario did not produce renderer output on stdout")
endif()
if(NOT LIVE_SCENARIO)
    string(ASCII 27 escape_character)
    string(FIND "${stdout}" "${escape_character}" escape_position)
    if(NOT escape_position EQUAL -1)
        message(FATAL_ERROR "Quiet scenario emitted ANSI terminal output")
    endif()
endif()
if(NOT "${result}" STREQUAL "${EXPECT_EXIT}")
    message(FATAL_ERROR
        "Scenario returned ${result}, expected ${EXPECT_EXIT}.\n"
        "Artifacts: ${ARTIFACT_DIR}\n--- diagnostics ---\n${stderr}")
endif()

if(DEFINED EXPECT_FAILURE_PATTERN)
    string(FIND "${output}" "${EXPECT_FAILURE_PATTERN}" pattern_position)
    if(pattern_position EQUAL -1)
        message(FATAL_ERROR
            "Scenario output did not contain: ${EXPECT_FAILURE_PATTERN}\n"
            "Artifacts: ${ARTIFACT_DIR}\n--- diagnostics ---\n${stderr}")
    endif()
endif()

set(result_file "${ARTIFACT_DIR}/result.json")
if(NOT EXISTS "${result_file}")
    message(FATAL_ERROR "Scenario did not create ${result_file}\n${stderr}")
endif()
file(READ "${result_file}" result_json)
string(JSON schema_version ERROR_VARIABLE json_error GET "${result_json}" schemaVersion)
if(json_error OR NOT schema_version EQUAL 1)
    message(FATAL_ERROR "Invalid Runner result schema: ${json_error}")
endif()
string(JSON result_exit GET "${result_json}" exitCode)
if(NOT "${result_exit}" STREQUAL "${EXPECT_EXIT}")
    message(FATAL_ERROR "result.json exitCode ${result_exit}, expected ${EXPECT_EXIT}")
endif()
string(JSON result_status GET "${result_json}" status)
if(EXPECT_EXIT EQUAL 0 AND NOT result_status STREQUAL "passed")
    message(FATAL_ERROR "result.json status ${result_status}, expected passed")
elseif(NOT EXPECT_EXIT EQUAL 0 AND NOT result_status STREQUAL "failed")
    message(FATAL_ERROR "result.json status ${result_status}, expected failed")
endif()
string(JSON result_mode GET "${result_json}" mode)
string(JSON result_scenario GET "${result_json}" scenario)
if(result_scenario MATCHES "/qmltui-(eval|capture)-[0-9]+\\.js$"
   AND EXISTS "${result_scenario}")
    message(FATAL_ERROR "Temporary ${result_mode} script was not removed: ${result_scenario}")
endif()

if(DEFINED EXPECT_ARTIFACT)
    set(artifact "${ARTIFACT_DIR}/${EXPECT_ARTIFACT}")
    if(NOT EXISTS "${artifact}")
        message(FATAL_ERROR "Expected artifact was not created: ${artifact}\n${stderr}")
    endif()
    if(DEFINED EXPECT_ARTIFACT_CONTENT)
        file(READ "${artifact}" artifact_contents)
        string(FIND "${artifact_contents}" "${EXPECT_ARTIFACT_CONTENT}" content_position)
        if(content_position EQUAL -1)
            message(FATAL_ERROR
                "Artifact ${artifact} did not contain: ${EXPECT_ARTIFACT_CONTENT}")
        endif()
    endif()
endif()

if(DEFINED FORBIDDEN_ARTIFACT AND EXISTS "${FORBIDDEN_ARTIFACT}")
    message(FATAL_ERROR "Forbidden artifact escaped its directory: ${FORBIDDEN_ARTIFACT}")
endif()

if(DEFINED EXPECT_UPDATED_SNAPSHOT)
    if(NOT EXISTS "${EXPECT_UPDATED_SNAPSHOT}")
        message(FATAL_ERROR
            "Update mode did not write the snapshot baseline: ${EXPECT_UPDATED_SNAPSHOT}")
    endif()
    if(DEFINED EXPECT_UPDATED_SNAPSHOT_CONTENT)
        file(READ "${EXPECT_UPDATED_SNAPSHOT}" updated_contents)
        string(FIND "${updated_contents}" "${EXPECT_UPDATED_SNAPSHOT_CONTENT}" updated_position)
        if(updated_position EQUAL -1)
            message(FATAL_ERROR
                "Updated snapshot ${EXPECT_UPDATED_SNAPSHOT} did not contain: "
                "${EXPECT_UPDATED_SNAPSHOT_CONTENT}")
        endif()
    endif()
    string(JSON updated_count LENGTH "${result_json}" artifacts updatedSnapshots)
    if(updated_count EQUAL 0)
        message(FATAL_ERROR "result.json artifacts.updatedSnapshots is empty")
    endif()
    string(FIND "${output}" "updated snapshots:" updated_summary_position)
    if(updated_summary_position EQUAL -1)
        message(FATAL_ERROR "Runner summary did not list the updated snapshots")
    endif()
endif()
