include_guard(GLOBAL)
include(CMakeParseArguments)

# Cached so every directory sees the path even though include_guard(GLOBAL)
# skips repeated includes from other directories.
set(QTUI_RUNNER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/RunQtuiScenario.cmake"
    CACHE INTERNAL "Path to the Qtui scenario runner script")
set(QTUI_TESTRUNNER_SCRIPT "${QTUI_RUNNER_SCRIPT}"
    CACHE INTERNAL "Legacy path to the Qtui scenario runner script")

function(qtui_add_scenario)
    cmake_parse_arguments(QTUI_SCENARIO
        "EMBEDDED;LIVE;UPDATE_SNAPSHOTS;NO_UPDATE_SNAPSHOTS"
        "NAME;TARGET;SCRIPT;COLUMNS;ROWS;OPERATION_TIMEOUT_MS;SCENARIO_TIMEOUT_MS;ARTIFACT_DIR;QML_IMPORT_PATH;EXPECTED_EXIT;FAILURE_PATTERN;ARTIFACT;ARTIFACT_CONTENT;FORBIDDEN_ARTIFACT;PROCESS_TIMEOUT;SYMLINK_TRAP;SNAPSHOT_SEED_SOURCE;SNAPSHOT_SEED_DEST;EXPECT_UPDATED_SNAPSHOT;EXPECT_UPDATED_SNAPSHOT_CONTENT;CLOCK"
        "LABELS"
        ${ARGN}
    )

    if(NOT QTUI_SCENARIO_NAME OR NOT QTUI_SCENARIO_TARGET)
        message(FATAL_ERROR "qtui_add_scenario requires NAME and TARGET")
    endif()
    if(NOT TARGET "${QTUI_SCENARIO_TARGET}")
        message(FATAL_ERROR "Unknown scenario TARGET: ${QTUI_SCENARIO_TARGET}")
    endif()
    if(QTUI_SCENARIO_EMBEDDED AND QTUI_SCENARIO_SCRIPT)
        message(FATAL_ERROR "Use exactly one of SCRIPT or EMBEDDED")
    endif()
    if(NOT QTUI_SCENARIO_EMBEDDED AND NOT QTUI_SCENARIO_SCRIPT)
        message(FATAL_ERROR "Use exactly one of SCRIPT or EMBEDDED")
    endif()

    foreach(default_pair
            "COLUMNS;80"
            "ROWS;24"
            "OPERATION_TIMEOUT_MS;2000"
            "SCENARIO_TIMEOUT_MS;10000"
            "EXPECTED_EXIT;0")
        list(GET default_pair 0 key)
        list(GET default_pair 1 value)
        if(NOT DEFINED QTUI_SCENARIO_${key} OR "${QTUI_SCENARIO_${key}}" STREQUAL "")
            set(QTUI_SCENARIO_${key} "${value}")
        endif()
    endforeach()
    if(NOT QTUI_SCENARIO_ARTIFACT_DIR)
        set(QTUI_SCENARIO_ARTIFACT_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/RunnerArtifacts/${QTUI_SCENARIO_NAME}")
    endif()
    if(NOT QTUI_SCENARIO_QML_IMPORT_PATH)
        # In Qtui's own build tree the modules live in the build output dir; for
        # find_package() consumers QtuiConfig exports the installed import root.
        if(QT_QML_OUTPUT_DIRECTORY)
            set(QTUI_SCENARIO_QML_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}")
        elseif(Qtui_QML_IMPORT_PATH)
            set(QTUI_SCENARIO_QML_IMPORT_PATH "${Qtui_QML_IMPORT_PATH}")
        endif()
    endif()
    if(NOT QTUI_SCENARIO_PROCESS_TIMEOUT)
        math(EXPR QTUI_SCENARIO_PROCESS_TIMEOUT
            "(${QTUI_SCENARIO_SCENARIO_TIMEOUT_MS} + 999) / 1000 + 5")
    endif()
    math(EXPR ctest_timeout "${QTUI_SCENARIO_PROCESS_TIMEOUT} + 3")

    set(command
        "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=$<TARGET_FILE:${QTUI_SCENARIO_TARGET}>"
        "-DARTIFACT_DIR=${QTUI_SCENARIO_ARTIFACT_DIR}"
        "-DEXPECT_EXIT=${QTUI_SCENARIO_EXPECTED_EXIT}"
        "-DPROCESS_TIMEOUT=${QTUI_SCENARIO_PROCESS_TIMEOUT}"
        "-DCOLUMNS=${QTUI_SCENARIO_COLUMNS}"
        "-DROWS=${QTUI_SCENARIO_ROWS}"
        "-DOPERATION_TIMEOUT_MS=${QTUI_SCENARIO_OPERATION_TIMEOUT_MS}"
        "-DSCENARIO_TIMEOUT_MS=${QTUI_SCENARIO_SCENARIO_TIMEOUT_MS}"
        "-DQML_IMPORT_PATH=${QTUI_SCENARIO_QML_IMPORT_PATH}"
    )
    if(QTUI_SCENARIO_EMBEDDED)
        list(APPEND command "-DEMBEDDED_SCENARIO=ON")
    else()
        list(APPEND command "-DSCENARIO_SCRIPT=${QTUI_SCENARIO_SCRIPT}")
    endif()
    if(QTUI_SCENARIO_LIVE)
        list(APPEND command "-DLIVE_SCENARIO=ON")
    endif()
    # QTUI_UPDATE_SNAPSHOTS is inherited from the caller's environment by
    # default, so `QTUI_UPDATE_SNAPSHOTS=1 ctest -R <name>` rewrites baselines.
    # UPDATE_SNAPSHOTS forces it on for a specific test; NO_UPDATE_SNAPSHOTS
    # forces it off so a verifying or negative test stays deterministic even
    # under a global update run.
    if(QTUI_SCENARIO_UPDATE_SNAPSHOTS AND QTUI_SCENARIO_NO_UPDATE_SNAPSHOTS)
        message(FATAL_ERROR "Use at most one of UPDATE_SNAPSHOTS and NO_UPDATE_SNAPSHOTS")
    endif()
    if(QTUI_SCENARIO_UPDATE_SNAPSHOTS)
        list(APPEND command "-DUPDATE_SNAPSHOTS=ON")
    elseif(QTUI_SCENARIO_NO_UPDATE_SNAPSHOTS)
        list(APPEND command "-DNO_UPDATE_SNAPSHOTS=ON")
    endif()
    if(QTUI_SCENARIO_SNAPSHOT_SEED_SOURCE OR QTUI_SCENARIO_SNAPSHOT_SEED_DEST)
        if(NOT QTUI_SCENARIO_SNAPSHOT_SEED_SOURCE OR NOT QTUI_SCENARIO_SNAPSHOT_SEED_DEST)
            message(FATAL_ERROR
                "SNAPSHOT_SEED_SOURCE and SNAPSHOT_SEED_DEST must be used together")
        endif()
        list(APPEND command
            "-DSNAPSHOT_SEED_SOURCE=${QTUI_SCENARIO_SNAPSHOT_SEED_SOURCE}"
            "-DSNAPSHOT_SEED_DEST=${QTUI_SCENARIO_SNAPSHOT_SEED_DEST}")
    endif()
    if(QTUI_SCENARIO_EXPECT_UPDATED_SNAPSHOT)
        list(APPEND command
            "-DEXPECT_UPDATED_SNAPSHOT=${QTUI_SCENARIO_EXPECT_UPDATED_SNAPSHOT}")
    endif()
    if(QTUI_SCENARIO_EXPECT_UPDATED_SNAPSHOT_CONTENT)
        list(APPEND command
            "-DEXPECT_UPDATED_SNAPSHOT_CONTENT=${QTUI_SCENARIO_EXPECT_UPDATED_SNAPSHOT_CONTENT}")
    endif()
    # CLOCK VIRTUAL runs the scenario on the Qtui virtual clock: application
    # time advances only through terminal.advanceTime() and terminal.sleep().
    if(QTUI_SCENARIO_CLOCK)
        string(TOLOWER "${QTUI_SCENARIO_CLOCK}" scenario_clock)
        if(NOT scenario_clock MATCHES "^(real|virtual)$")
            message(FATAL_ERROR "CLOCK must be REAL or VIRTUAL, got '${QTUI_SCENARIO_CLOCK}'")
        endif()
        list(APPEND command "-DCLOCK=${scenario_clock}")
    endif()
    foreach(argument FAILURE_PATTERN ARTIFACT ARTIFACT_CONTENT)
        if(NOT "${QTUI_SCENARIO_${argument}}" STREQUAL "")
            list(APPEND command "-DEXPECT_${argument}=${QTUI_SCENARIO_${argument}}")
        endif()
    endforeach()
    if(QTUI_SCENARIO_FORBIDDEN_ARTIFACT)
        list(APPEND command
            "-DFORBIDDEN_ARTIFACT=${QTUI_SCENARIO_FORBIDDEN_ARTIFACT}")
    endif()
    if(QTUI_SCENARIO_SYMLINK_TRAP)
        list(APPEND command "-DSYMLINK_TRAP=${QTUI_SCENARIO_SYMLINK_TRAP}")
    endif()
    list(APPEND command "-P" "${QTUI_RUNNER_SCRIPT}")

    add_test(NAME "${QTUI_SCENARIO_NAME}" COMMAND ${command})
    set(labels Runner)
    list(APPEND labels ${QTUI_SCENARIO_LABELS})
    list(REMOVE_DUPLICATES labels)
    set_tests_properties("${QTUI_SCENARIO_NAME}" PROPERTIES
        SKIP_RETURN_CODE 125
        TIMEOUT ${ctest_timeout}
        LABELS "${labels}"
    )
endfunction()

function(qtui_add_scenario_test)
    qtui_add_scenario(${ARGN})
endfunction()
