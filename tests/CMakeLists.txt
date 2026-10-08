include(CTest)
enable_testing()
include("${Qtui_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")

# One directory per library, mirroring src/Qtui: Core, Controls, Image,
# TestRunner and TerminalHost each register their own tests. Examples/ holds
# the scenario runs against the example applications, and the installed
# consumer below is a separate CMake project built against an install tree.
# The helpers here are shared by every subdirectory.

# A Qt Test executable. Every test links the three always-built libraries so
# a Core test can instantiate a control or an image without ceremony.
function(add_qtui_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE
        Qt6::Core Qt6::Gui Qt6::Test Qtui::Core Qtui::Controls Qtui::Image)
    target_compile_definitions(${test_name} PRIVATE QTUI_SOURCE_DIR="${Qtui_SOURCE_DIR}")
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
    )
endfunction()

set(qtui_test_runner_artifacts
    "${CMAKE_CURRENT_BINARY_DIR}/TestRunnerArtifacts")

# A TestRunner scenario driven by a script (or an embedded resource), with
# bounded timeouts and the artifact checks the runner contract promises.
function(add_qtui_runner_scenario test_name executable script expected_exit)
    cmake_parse_arguments(SCENARIO "EMBEDDED;LIVE;FULL"
        "FAILURE_PATTERN;ARTIFACT;ARTIFACT_CONTENT;FORBIDDEN_ARTIFACT;PROCESS_TIMEOUT;SYMLINK_TRAP;OPERATION_TIMEOUT_MS;SCENARIO_TIMEOUT_MS"
        "" ${ARGN})
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
        ARTIFACT_DIR "${qtui_test_runner_artifacts}/${test_name}"
        OPERATION_TIMEOUT_MS ${SCENARIO_OPERATION_TIMEOUT_MS}
        SCENARIO_TIMEOUT_MS ${SCENARIO_SCENARIO_TIMEOUT_MS}
    )
    if(SCENARIO_EMBEDDED)
        list(APPEND arguments EMBEDDED)
    else()
        list(APPEND arguments SCRIPT "${script}")
    endif()
    if(SCENARIO_LIVE)
        list(APPEND arguments LIVE)
    endif()
    if(SCENARIO_FULL)
        list(APPEND arguments LABELS AgentFull)
    else()
        list(APPEND arguments LABELS AgentSmoke)
    endif()
    foreach(argument FAILURE_PATTERN ARTIFACT ARTIFACT_CONTENT FORBIDDEN_ARTIFACT PROCESS_TIMEOUT SYMLINK_TRAP)
        if(NOT "${SCENARIO_${argument}}" STREQUAL "")
            list(APPEND arguments ${argument} "${SCENARIO_${argument}}")
        endif()
    endforeach()
    qtui_add_scenario_test(${arguments})
endfunction()

# A TestRunner session requested through the environment (QTUI_CAPTURE_AFTER_MS,
# QTUI_EVAL, or a deliberate conflict between modes).
function(add_qtui_environment_scenario test_name executable expected_exit)
    cmake_parse_arguments(SCENARIO "CONFLICT"
        "CAPTURE_AFTER_MS;SCRIPT;EVAL_CODE;FAILURE_PATTERN;ARTIFACT;ARTIFACT_CONTENT" "" ${ARGN})
    set(command
        "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=$<TARGET_FILE:${executable}>"
        "-DARTIFACT_DIR=${qtui_test_runner_artifacts}/${test_name}"
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
    elseif(SCENARIO_EVAL_CODE)
        list(APPEND command "-DEVAL_CODE=${SCENARIO_EVAL_CODE}")
    else()
        message(FATAL_ERROR "Environment scenario requires CAPTURE_AFTER_MS or EVAL_CODE")
    endif()
    foreach(argument FAILURE_PATTERN ARTIFACT ARTIFACT_CONTENT)
        if(SCENARIO_${argument})
            list(APPEND command "-DEXPECT_${argument}=${SCENARIO_${argument}}")
        endif()
    endforeach()
    list(APPEND command "-P" "${QTUI_TESTRUNNER_SCRIPT}")
    add_test(NAME ${test_name} COMMAND ${command})
    set_tests_properties(${test_name} PROPERTIES
        SKIP_RETURN_CODE 125
        TIMEOUT 15
        LABELS "TestRunner;AgentSmoke"
    )
endfunction()

add_subdirectory(Core)
add_subdirectory(Controls)
add_subdirectory(Image)
add_subdirectory(TestRunner)
if(QTUI_ENABLE_TERMINAL_HOST)
    add_subdirectory(TerminalHost)
endif()
add_subdirectory(Examples)

# Installs this build into a private prefix, then configures, builds and runs
# tests/installed_consumer as an independent project against it: find_package,
# every installed header, and a custom item painting through the public API.
add_test(
    NAME test_installed_custom_item_consumer
    COMMAND "${CMAKE_COMMAND}"
        "-DQTUI_BUILD_DIR=${CMAKE_BINARY_DIR}"
        "-DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/installed_consumer"
        "-DCONSUMER_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/installed_consumer_build"
        "-DINSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/installed_consumer_prefix"
        "-DQt6_DIR=${Qt6_DIR}"
        "-DCONSUMER_GENERATOR=${CMAKE_GENERATOR}"
        "-DCONSUMER_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/installed_consumer/RunConsumer.cmake"
)
set_tests_properties(test_installed_custom_item_consumer PROPERTIES
    LABELS AgentSmoke
    TIMEOUT 120
)
