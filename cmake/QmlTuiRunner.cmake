include_guard(GLOBAL)
include(CMakeParseArguments)

# Cached so every directory sees the path even though include_guard(GLOBAL)
# skips repeated includes from other directories.
set(QMLTUI_RUNNER_SCENARIO_DRIVER "${CMAKE_CURRENT_LIST_DIR}/RunQmlTuiScenario.cmake"
    CACHE INTERNAL "Path to the CMake script that launches and validates one Runner scenario" FORCE)

function(qmltui_add_scenario)
    cmake_parse_arguments(QMLTUI_SCENARIO
        "EMBEDDED;LIVE;UPDATE_SNAPSHOTS;NO_UPDATE_SNAPSHOTS"
        "NAME;TARGET;SCRIPT;COLUMNS;ROWS;OPERATION_TIMEOUT_MS;SCENARIO_TIMEOUT_MS;ARTIFACT_DIR;QML_IMPORT_PATH;EXPECTED_EXIT;FAILURE_PATTERN;ARTIFACT;ARTIFACT_CONTENT;FORBIDDEN_ARTIFACT;PROCESS_TIMEOUT;SYMLINK_TRAP;SNAPSHOT_SEED_SOURCE;SNAPSHOT_SEED_DEST;EXPECT_UPDATED_SNAPSHOT;EXPECT_UPDATED_SNAPSHOT_CONTENT;CLOCK;RECORD"
        "LABELS;ARGUMENTS;POST_COMMAND"
        ${ARGN}
    )

    if(NOT QMLTUI_SCENARIO_NAME OR NOT QMLTUI_SCENARIO_TARGET)
        message(FATAL_ERROR "qmltui_add_scenario requires NAME and TARGET")
    endif()
    if(NOT TARGET "${QMLTUI_SCENARIO_TARGET}")
        message(FATAL_ERROR "Unknown scenario TARGET: ${QMLTUI_SCENARIO_TARGET}")
    endif()
    if(QMLTUI_SCENARIO_EMBEDDED AND QMLTUI_SCENARIO_SCRIPT)
        message(FATAL_ERROR "Use exactly one of SCRIPT or EMBEDDED")
    endif()
    if(NOT QMLTUI_SCENARIO_EMBEDDED AND NOT QMLTUI_SCENARIO_SCRIPT)
        message(FATAL_ERROR "Use exactly one of SCRIPT or EMBEDDED")
    endif()
    if(QMLTUI_SCENARIO_RECORD AND QMLTUI_SCENARIO_LIVE)
        message(FATAL_ERROR "RECORD captures the terminal bytes in memory; it cannot be combined with LIVE")
    endif()

    foreach(default_pair
            "COLUMNS;80"
            "ROWS;24"
            "OPERATION_TIMEOUT_MS;2000"
            "SCENARIO_TIMEOUT_MS;30000"
            "EXPECTED_EXIT;0")
        list(GET default_pair 0 key)
        list(GET default_pair 1 value)
        if(NOT DEFINED QMLTUI_SCENARIO_${key} OR "${QMLTUI_SCENARIO_${key}}" STREQUAL "")
            set(QMLTUI_SCENARIO_${key} "${value}")
        endif()
    endforeach()
    if(NOT QMLTUI_SCENARIO_ARTIFACT_DIR)
        set(QMLTUI_SCENARIO_ARTIFACT_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/RunnerArtifacts/${QMLTUI_SCENARIO_NAME}")
    endif()
    if(NOT QMLTUI_SCENARIO_QML_IMPORT_PATH)
        # In QmlTui's own build tree the modules live in the build output dir; for
        # find_package() consumers QmlTuiConfig exports the installed import root.
        # Include both when a find_package() consumer also sets QT_QML_OUTPUT_DIRECTORY.
        set(_scenario_import_dirs)
        if(QT_QML_OUTPUT_DIRECTORY)
            list(APPEND _scenario_import_dirs "${QT_QML_OUTPUT_DIRECTORY}")
        endif()
        if(QmlTui_QML_IMPORT_PATH)
            list(APPEND _scenario_import_dirs "${QmlTui_QML_IMPORT_PATH}")
        endif()
        list(REMOVE_DUPLICATES _scenario_import_dirs)
        if(WIN32)
            string(REPLACE ";" "\\;" QMLTUI_SCENARIO_QML_IMPORT_PATH "${_scenario_import_dirs}")
        else()
            string(JOIN ":" QMLTUI_SCENARIO_QML_IMPORT_PATH ${_scenario_import_dirs})
        endif()
    endif()
    if(NOT QMLTUI_SCENARIO_PROCESS_TIMEOUT)
        math(EXPR QMLTUI_SCENARIO_PROCESS_TIMEOUT
            "(${QMLTUI_SCENARIO_SCENARIO_TIMEOUT_MS} + 999) / 1000 + 5")
    endif()
    math(EXPR ctest_timeout "${QMLTUI_SCENARIO_PROCESS_TIMEOUT} + 3")

    set(command
        "${CMAKE_COMMAND}"
        "-DSCENARIO_NAME=${QMLTUI_SCENARIO_NAME}"
        "-DTEST_EXECUTABLE=$<TARGET_FILE:${QMLTUI_SCENARIO_TARGET}>"
        "-DARTIFACT_DIR=${QMLTUI_SCENARIO_ARTIFACT_DIR}"
        "-DEXPECT_EXIT=${QMLTUI_SCENARIO_EXPECTED_EXIT}"
        "-DPROCESS_TIMEOUT=${QMLTUI_SCENARIO_PROCESS_TIMEOUT}"
        "-DCOLUMNS=${QMLTUI_SCENARIO_COLUMNS}"
        "-DROWS=${QMLTUI_SCENARIO_ROWS}"
        "-DOPERATION_TIMEOUT_MS=${QMLTUI_SCENARIO_OPERATION_TIMEOUT_MS}"
        "-DSCENARIO_TIMEOUT_MS=${QMLTUI_SCENARIO_SCENARIO_TIMEOUT_MS}"
        "-DQML_IMPORT_PATH=${QMLTUI_SCENARIO_QML_IMPORT_PATH}"
    )
    if(QMLTUI_SCENARIO_EMBEDDED)
        list(APPEND command "-DEMBEDDED_SCENARIO=ON")
    else()
        list(APPEND command "-DSCENARIO_SCRIPT=${QMLTUI_SCENARIO_SCRIPT}")
    endif()
    if(QMLTUI_SCENARIO_LIVE)
        list(APPEND command "-DLIVE_SCENARIO=ON")
    endif()
    # QMLTUI_RUNNER_UPDATE_SNAPSHOTS is inherited from the caller's environment by
    # default, so `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest -R <name>` rewrites baselines.
    # UPDATE_SNAPSHOTS forces it on for a specific test; NO_UPDATE_SNAPSHOTS
    # forces it off so a verifying or negative test stays deterministic even
    # under a global update run.
    if(QMLTUI_SCENARIO_UPDATE_SNAPSHOTS AND QMLTUI_SCENARIO_NO_UPDATE_SNAPSHOTS)
        message(FATAL_ERROR "Use at most one of UPDATE_SNAPSHOTS and NO_UPDATE_SNAPSHOTS")
    endif()
    if(QMLTUI_SCENARIO_UPDATE_SNAPSHOTS)
        list(APPEND command "-DUPDATE_SNAPSHOTS=ON")
    elseif(QMLTUI_SCENARIO_NO_UPDATE_SNAPSHOTS)
        list(APPEND command "-DNO_UPDATE_SNAPSHOTS=ON")
    endif()
    if(QMLTUI_SCENARIO_SNAPSHOT_SEED_SOURCE OR QMLTUI_SCENARIO_SNAPSHOT_SEED_DEST)
        if(NOT QMLTUI_SCENARIO_SNAPSHOT_SEED_SOURCE OR NOT QMLTUI_SCENARIO_SNAPSHOT_SEED_DEST)
            message(FATAL_ERROR
                "SNAPSHOT_SEED_SOURCE and SNAPSHOT_SEED_DEST must be used together")
        endif()
        list(APPEND command
            "-DSNAPSHOT_SEED_SOURCE=${QMLTUI_SCENARIO_SNAPSHOT_SEED_SOURCE}"
            "-DSNAPSHOT_SEED_DEST=${QMLTUI_SCENARIO_SNAPSHOT_SEED_DEST}")
    endif()
    if(QMLTUI_SCENARIO_EXPECT_UPDATED_SNAPSHOT)
        list(APPEND command
            "-DEXPECT_UPDATED_SNAPSHOT=${QMLTUI_SCENARIO_EXPECT_UPDATED_SNAPSHOT}")
    endif()
    if(QMLTUI_SCENARIO_EXPECT_UPDATED_SNAPSHOT_CONTENT)
        list(APPEND command
            "-DEXPECT_UPDATED_SNAPSHOT_CONTENT=${QMLTUI_SCENARIO_EXPECT_UPDATED_SNAPSHOT_CONTENT}")
    endif()
    # CLOCK VIRTUAL runs the scenario on the QmlTui virtual clock: application
    # time advances only through terminal.advanceTime() and terminal.sleep().
    if(QMLTUI_SCENARIO_CLOCK)
        string(TOLOWER "${QMLTUI_SCENARIO_CLOCK}" scenario_clock)
        if(NOT scenario_clock MATCHES "^(real|virtual)$")
            message(FATAL_ERROR "CLOCK must be REAL or VIRTUAL, got '${QMLTUI_SCENARIO_CLOCK}'")
        endif()
        list(APPEND command "-DCLOCK=${scenario_clock}")
    endif()
    # RECORD <name.cast> writes an asciicast of the session (into
    # QMLTUI_RECORD_DIR when set, else ARTIFACT_DIR) and labels the test Demo
    # by default instead of Runner.
    if(QMLTUI_SCENARIO_RECORD)
        if(IS_ABSOLUTE "${QMLTUI_SCENARIO_RECORD}" OR NOT QMLTUI_RECORD_DIR)
            list(APPEND command "-DRECORD=${QMLTUI_SCENARIO_RECORD}")
        else()
            list(APPEND command "-DRECORD=${QMLTUI_RECORD_DIR}/${QMLTUI_SCENARIO_RECORD}")
        endif()
    endif()
    foreach(argument FAILURE_PATTERN ARTIFACT ARTIFACT_CONTENT)
        if(NOT "${QMLTUI_SCENARIO_${argument}}" STREQUAL "")
            list(APPEND command "-DEXPECT_${argument}=${QMLTUI_SCENARIO_${argument}}")
        endif()
    endforeach()
    if(QMLTUI_SCENARIO_FORBIDDEN_ARTIFACT)
        list(APPEND command
            "-DFORBIDDEN_ARTIFACT=${QMLTUI_SCENARIO_FORBIDDEN_ARTIFACT}")
    endif()
    if(QMLTUI_SCENARIO_SYMLINK_TRAP)
        list(APPEND command "-DSYMLINK_TRAP=${QMLTUI_SCENARIO_SYMLINK_TRAP}")
    endif()
    # ARGUMENTS are appended to the executable's command line, for a harness
    # that selects its scenario or options from argv rather than the environment.
    if(QMLTUI_SCENARIO_ARGUMENTS)
        # Escaped so the list survives as one -D value and reappears as a
        # list inside the driver script.
        string(REPLACE ";" "\\;" scenario_arguments "${QMLTUI_SCENARIO_ARGUMENTS}")
        list(APPEND command "-DTEST_ARGUMENTS=${scenario_arguments}")
    endif()
    # POST_COMMAND (or the directory/project default QMLTUI_RECORD_POST_COMMAND
    # when RECORD is set) runs after the scenario and result validation succeed.
    # Placeholders <CAST>, <CAST_STEM>, <ARTIFACT_DIR>, and <NAME> are expanded
    # by the driver script.
    if(NOT QMLTUI_SCENARIO_POST_COMMAND AND QMLTUI_SCENARIO_RECORD AND QMLTUI_RECORD_POST_COMMAND)
        set(QMLTUI_SCENARIO_POST_COMMAND ${QMLTUI_RECORD_POST_COMMAND})
    endif()
    if(QMLTUI_SCENARIO_POST_COMMAND)
        list(LENGTH QMLTUI_SCENARIO_POST_COMMAND post_command_length)
        if(post_command_length EQUAL 1 AND QMLTUI_SCENARIO_POST_COMMAND MATCHES "[ \t]")
            separate_arguments(QMLTUI_SCENARIO_POST_COMMAND NATIVE_COMMAND
                "${QMLTUI_SCENARIO_POST_COMMAND}")
        endif()
        string(REPLACE ";" "\\;" scenario_post_command "${QMLTUI_SCENARIO_POST_COMMAND}")
        list(APPEND command "-DPOST_COMMAND=${scenario_post_command}")
    endif()
    list(APPEND command "-P" "${QMLTUI_RUNNER_SCENARIO_DRIVER}")

    add_test(NAME "${QMLTUI_SCENARIO_NAME}" COMMAND ${command})
    if(QMLTUI_SCENARIO_LABELS)
        set(labels ${QMLTUI_SCENARIO_LABELS})
    elseif(QMLTUI_SCENARIO_RECORD)
        set(labels Demo)
    else()
        set(labels Runner)
    endif()
    list(REMOVE_DUPLICATES labels)
    set_tests_properties("${QMLTUI_SCENARIO_NAME}" PROPERTIES
        SKIP_RETURN_CODE 125
        TIMEOUT ${ctest_timeout}
        LABELS "${labels}"
    )
endfunction()

function(qmltui_add_scenario_test)
    qmltui_add_scenario(${ARGN})
endfunction()

# Materializes editor completion for Runner scenarios into a consumer project:
# copies the scenario API declarations to <DIRECTORY>/.qmltui/qmltui-runner.d.ts
# (with a .gitignore so the directory stays out of version control) and, when
# the project has no jsconfig.json yet, generates one that makes the
# declarations ambient for every *.js/*.mjs under DIRECTORY. An existing
# jsconfig.json is never touched; a message explains the one line to add.
# DIRECTORY defaults to the calling project's source directory. Works for
# find_package(), FetchContent, and add_subdirectory consumers alike.
function(qmltui_scenario_editor_support)
    cmake_parse_arguments(QMLTUI_EDITOR "" "DIRECTORY" "" ${ARGN})
    if(NOT QMLTUI_EDITOR_DIRECTORY)
        set(QMLTUI_EDITOR_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    if(NOT QMLTUI_RUNNER_DECLARATIONS OR NOT EXISTS "${QMLTUI_RUNNER_DECLARATIONS}")
        message(FATAL_ERROR
            "qmltui_scenario_editor_support: scenario API declarations not found "
            "(QMLTUI_RUNNER_DECLARATIONS='${QMLTUI_RUNNER_DECLARATIONS}'). Build or "
            "install QmlTui with the Runner enabled first.")
    endif()

    set(support_dir "${QMLTUI_EDITOR_DIRECTORY}/.qmltui")
    # configure_file(COPYONLY) rewrites only on change, so a warm reconfigure
    # leaves editor state alone.
    configure_file("${QMLTUI_RUNNER_DECLARATIONS}" "${support_dir}/qmltui-runner.d.ts" COPYONLY)
    if(NOT EXISTS "${support_dir}/.gitignore")
        file(WRITE "${support_dir}/.gitignore" "*\n")
    endif()

    set(tsconfig "${QMLTUI_EDITOR_DIRECTORY}/tsconfig.json")
    set(jsconfig "${QMLTUI_EDITOR_DIRECTORY}/jsconfig.json")
    if(EXISTS "${tsconfig}")
        file(READ "${tsconfig}" tsconfig_contents)
        string(FIND "${tsconfig_contents}" "qmltui-runner.d.ts" declarations_position)
        if(declarations_position EQUAL -1)
            message(STATUS
                "QmlTui: add \".qmltui/qmltui-runner.d.ts\" to the include list of "
                "${tsconfig} to get app.* completion in scenarios")
        endif()
    elseif(NOT EXISTS "${jsconfig}")
        file(WRITE "${jsconfig}" [=[{
    "compilerOptions": {
        "target": "es2020",
        "lib": ["es2020"],
        "module": "esnext",
        "moduleResolution": "bundler",
        "checkJs": false,
        "noEmit": true
    },
    "include": [
        ".qmltui/qmltui-runner.d.ts",
        "**/*.js",
        "**/*.mjs"
    ],
    "exclude": [
        "build",
        "build-*",
        "node_modules"
    ]
}
]=])
        message(STATUS
            "QmlTui: generated ${jsconfig}; scenarios get app.* completion "
            "from /** @param {Runner} app */")
    else()
        file(READ "${jsconfig}" jsconfig_contents)
        string(FIND "${jsconfig_contents}" "qmltui-runner.d.ts" declarations_position)
        if(declarations_position EQUAL -1)
            message(STATUS
                "QmlTui: add \".qmltui/qmltui-runner.d.ts\" to the include list of "
                "${jsconfig} to get app.* completion in scenarios")
        endif()
    endif()
endfunction()
