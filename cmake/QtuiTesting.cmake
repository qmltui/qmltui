include_guard(GLOBAL)
include(CMakeParseArguments)

set(QTUI_TESTING_RUNNER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/RunQtuiScenario.cmake")

function(qtui_add_scenario_test)
    cmake_parse_arguments(QTUI_SCENARIO
        "EMBEDDED;LIVE"
        "NAME;TARGET;SCRIPT;COLUMNS;ROWS;OPERATION_TIMEOUT_MS;SCENARIO_TIMEOUT_MS;ARTIFACT_DIR;QML_IMPORT_PATH;EXPECTED_EXIT;FAILURE_PATTERN;ARTIFACT;ARTIFACT_CONTENT;FORBIDDEN_ARTIFACT;PROCESS_TIMEOUT"
        "LABELS"
        ${ARGN}
    )

    if(NOT QTUI_SCENARIO_NAME OR NOT QTUI_SCENARIO_TARGET)
        message(FATAL_ERROR "qtui_add_scenario_test requires NAME and TARGET")
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
            "${CMAKE_CURRENT_BINARY_DIR}/TestRunnerArtifacts/${QTUI_SCENARIO_NAME}")
    endif()
    if(NOT QTUI_SCENARIO_QML_IMPORT_PATH)
        set(QTUI_SCENARIO_QML_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}")
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
    foreach(argument FAILURE_PATTERN ARTIFACT ARTIFACT_CONTENT)
        if(NOT "${QTUI_SCENARIO_${argument}}" STREQUAL "")
            list(APPEND command "-DEXPECT_${argument}=${QTUI_SCENARIO_${argument}}")
        endif()
    endforeach()
    if(QTUI_SCENARIO_FORBIDDEN_ARTIFACT)
        list(APPEND command
            "-DFORBIDDEN_ARTIFACT=${QTUI_SCENARIO_FORBIDDEN_ARTIFACT}")
    endif()
    list(APPEND command "-P" "${QTUI_TESTING_RUNNER_SCRIPT}")

    add_test(NAME "${QTUI_SCENARIO_NAME}" COMMAND ${command})
    set(labels TestRunner)
    list(APPEND labels ${QTUI_SCENARIO_LABELS})
    list(REMOVE_DUPLICATES labels)
    set_tests_properties("${QTUI_SCENARIO_NAME}" PROPERTIES
        TIMEOUT ${ctest_timeout}
        LABELS "${labels}"
    )
endfunction()
