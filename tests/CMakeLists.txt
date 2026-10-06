include(CTest)
enable_testing()
include("${Qtui_SOURCE_DIR}/cmake/QtuiTesting.cmake")

function(add_qtui_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE Qt6::Core Qt6::Gui Qt6::Test Qtui::Core Qtui::Controls Qtui::Extras Qtui::Canvas Qtui::Image)
    target_compile_definitions(${test_name} PRIVATE QTUI_SOURCE_DIR="${Qtui_SOURCE_DIR}")
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
    )
endfunction()


add_qtui_test(test_input_decoder Core/test_input_decoder.cpp)
add_qtui_test(test_text_grid_buffer Core/test_text_grid_buffer.cpp)
add_qtui_test(test_layout Core/test_layout.cpp)
add_qtui_test(test_loader Core/test_loader.cpp)
add_qtui_test(test_focus Core/test_focus.cpp)
add_qtui_test(test_text_input Core/test_text_input.cpp)
add_qtui_test(test_scroll_view Core/test_scroll_view.cpp)
add_qtui_test(test_views_virtualization Core/test_views_virtualization.cpp)
add_qtui_test(test_table_view_hardening Core/test_table_view_hardening.cpp)
add_qtui_test(test_terminal_framework Core/test_terminal_framework.cpp)
add_qtui_test(test_input_router Core/test_input_router.cpp)
add_qtui_test(test_unicode_text Core/test_unicode_text.cpp)
add_qtui_test(test_render_loop Core/test_render_loop.cpp)
add_qtui_test(test_text_layout Core/test_text_layout.cpp)
add_qtui_test(test_terminal_capabilities Core/test_terminal_capabilities.cpp)
add_qtui_test(test_color_depth Core/test_color_depth.cpp)
if(UNIX)
    add_qtui_test(test_terminal_device Core/test_terminal_device.cpp)
    set_tests_properties(test_terminal_device PROPERTIES TIMEOUT 20)
endif()

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

add_qtui_test(test_canvas Canvas/test_canvas.cpp)
add_qtui_test(test_popup Controls/test_popup.cpp)
add_qtui_test(test_extras Extras/test_extras.cpp)
add_qtui_test(test_image Image/test_image.cpp)

add_executable(qtui_test_runner_fixture Testing/main.cpp)
target_link_libraries(qtui_test_runner_fixture PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Testing
)
target_compile_definitions(qtui_test_runner_fixture PRIVATE
    QTUI_TEST_RUNNER_FIXTURE_QML="${CMAKE_CURRENT_SOURCE_DIR}/Testing/Fixture.qml"
)
qt_add_resources(qtui_test_runner_fixture embedded_test_runner_scenarios
    PREFIX "/qtui-tests"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Testing/scenarios"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Testing/scenarios/resource_snapshot.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/Testing/scenarios/snapshots/fixture.txt"
)

set(qtui_test_runner_artifacts
    "${CMAKE_CURRENT_BINARY_DIR}/TestRunnerArtifacts")

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

function(add_qtui_environment_scenario test_name executable expected_exit)
    cmake_parse_arguments(SCENARIO "INSPECT;INSPECT_IMAGE;CONFLICT"
        "SCRIPT;EVAL_CODE;FAILURE_PATTERN;ARTIFACT;ARTIFACT_CONTENT" "" ${ARGN})
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
    elseif(SCENARIO_INSPECT)
        list(APPEND command "-DINSPECT_SCENARIO=ON")
        if(SCENARIO_INSPECT_IMAGE)
            list(APPEND command "-DINSPECT_IMAGE=ON")
        endif()
    elseif(SCENARIO_EVAL_CODE)
        list(APPEND command "-DEVAL_CODE=${SCENARIO_EVAL_CODE}")
    else()
        message(FATAL_ERROR "Environment scenario requires INSPECT or EVAL_CODE")
    endif()
    foreach(argument FAILURE_PATTERN ARTIFACT ARTIFACT_CONTENT)
        if(SCENARIO_${argument})
            list(APPEND command "-DEXPECT_${argument}=${SCENARIO_${argument}}")
        endif()
    endforeach()
    list(APPEND command "-P" "${QTUI_TESTING_RUNNER_SCRIPT}")
    add_test(NAME ${test_name} COMMAND ${command})
    set_tests_properties(${test_name} PROPERTIES
        SKIP_RETURN_CODE 125
        TIMEOUT 15
        LABELS "TestRunner;AgentSmoke"
    )
endfunction()

set(qtui_test_runner_scenarios
    "${CMAKE_CURRENT_SOURCE_DIR}/Testing/scenarios")

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

add_qtui_environment_scenario(test_runner_inspect qtui_pos_cashier 0
    INSPECT
    ARTIFACT "inspect.txt"
    ARTIFACT_CONTENT "RapidPOS"
)
add_qtui_environment_scenario(test_runner_inspect_image_unsupported qtui_pos_cashier 1
    INSPECT
    INSPECT_IMAGE
    FAILURE_PATTERN "Image capture requires QGuiApplication"
    ARTIFACT "failure.txt"
)
add_qtui_environment_scenario(test_runner_eval qtui_test_runner_fixture 0
    EVAL_CODE "Test.captureText('eval.txt')"
    ARTIFACT "eval.txt"
    ARTIFACT_CONTENT "TESTRUNNER FIXTURE"
)
add_qtui_environment_scenario(test_runner_environment_conflict qtui_test_runner_fixture 1
    CONFLICT
    SCRIPT "${qtui_test_runner_scenarios}/success.js"
    EVAL_CODE "Test.frameText()"
    FAILURE_PATTERN "mutually exclusive"
    ARTIFACT "result.json"
)

if(TARGET qtui_pos_cashier)
    add_qtui_runner_scenario(test_runner_pos qtui_pos_cashier
        "${Qtui_SOURCE_DIR}/examples/apps/pos_cashier/test_pos.js" 0
        ARTIFACT "pos/receipt.txt"
        ARTIFACT_CONTENT "TRANSACTION COMPLETED"
    )
endif()
if(TARGET qtui_snake_game)
    add_qtui_runner_scenario(test_runner_snake qtui_snake_game
        "${Qtui_SOURCE_DIR}/examples/apps/snake_game/test_snake.js" 0
        ARTIFACT "snake/paused.txt"
        ARTIFACT_CONTENT "PAUSED"
    )
    add_qtui_runner_scenario(test_runner_snake_ai qtui_snake_game
        "${Qtui_SOURCE_DIR}/examples/apps/snake_game/test_snake_ai.js" 0
        FULL
        SCENARIO_TIMEOUT_MS 15000
        ARTIFACT "snake/ai_gameplay.txt"
        ARTIFACT_CONTENT "SCORE"
    )
endif()
if(TARGET qtui_container_fleet)
    add_qtui_runner_scenario(test_runner_container_fleet qtui_container_fleet
        "${Qtui_SOURCE_DIR}/examples/apps/container_fleet/test_fleet.js" 0
        ARTIFACT "fleet/critical.txt"
        ARTIFACT_CONTENT "CRITICAL"
    )
endif()
if(TARGET qtui_pacman_game)
    add_qtui_runner_scenario(test_runner_pacman qtui_pacman_game
        "${Qtui_SOURCE_DIR}/examples/apps/pacman_game/test_pacman.js" 0
        ARTIFACT "pacman/paused.txt"
        ARTIFACT_CONTENT "PAUSED"
    )
endif()
if(TARGET qtui_sql_studio)
    add_qtui_runner_scenario(test_runner_sql_studio qtui_sql_studio
        "${Qtui_SOURCE_DIR}/examples/apps/sql_studio/test_sql_studio.js" 0
        ARTIFACT "sql_studio_verified.txt"
        ARTIFACT_CONTENT "SQLStudio TUI"
    )
endif()
if(TARGET qtui_industrial_plc_hmi)
    add_qtui_runner_scenario(test_runner_plc qtui_industrial_plc_hmi
        "${Qtui_SOURCE_DIR}/examples/apps/industrial_plc_hmi/test_plc.js" 0
        FULL
        PROCESS_TIMEOUT 35
        OPERATION_TIMEOUT_MS 5000
        SCENARIO_TIMEOUT_MS 30000
        ARTIFACT "plc/reset.txt"
        ARTIFACT_CONTENT "STATE: IDLE (ALARM RESET)"
    )
endif()
if(TARGET qtui_twozero48)
    add_qtui_runner_scenario(test_runner_twozero48 qtui_twozero48
        "${Qtui_SOURCE_DIR}/examples/apps/twozero48/test_twozero48.js" 0
        ARTIFACT "twozero48/initial.txt"
        ARTIFACT_CONTENT "2048 TUI"
    )
endif()

if(QTUI_BUILD_TERMINAL_HOST)
    add_executable(test_terminal_host TerminalHost/test_terminal_host.cpp)
    target_link_libraries(test_terminal_host PRIVATE
        Qt6::Core Qt6::Test Qt6::Qml Qtui::TerminalHost qtui_libvterm
    )
    target_compile_definitions(test_terminal_host PRIVATE
        QTUI_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
    )
    add_test(NAME test_terminal_host COMMAND test_terminal_host)
endif()

if(QTUI_BUILD_BENCHMARKS)
    add_executable(benchmark_table_view Core/benchmark_table_view.cpp)
    target_link_libraries(benchmark_table_view PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

    add_executable(benchmark_views Core/benchmark_views.cpp)
    target_link_libraries(benchmark_views PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::Extras Qtui::Canvas)

    # Per-frame paint/diff cost for representative scenes. Informational; run
    # it before and after rendering changes and record the numbers.
    add_executable(benchmark_render Core/benchmark_render.cpp)
    target_link_libraries(benchmark_render PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::Extras Qtui::Canvas)
endif()
