# Tests for Qamui::Core: primitives, engine, items, and the benchmarks.

add_qamui_test(test_input_decoder test_input_decoder.cpp)
add_qamui_test(test_text_grid_buffer test_text_grid_buffer.cpp)
add_qamui_test(test_unicode_text test_unicode_text.cpp)
add_qamui_test(test_color_depth test_color_depth.cpp)
add_qamui_test(test_terminal_capabilities test_terminal_capabilities.cpp)
add_qamui_test(test_terminal_backend test_terminal_backend.cpp)
target_link_libraries(test_terminal_backend PRIVATE Qamui::Runner)
add_qamui_test(test_terminal_queries test_terminal_queries.cpp)
target_link_libraries(test_terminal_queries PRIVATE Qamui::Runner)
add_qamui_test(test_clock_virtual test_clock_virtual.cpp)
add_qamui_test(test_clock_real test_clock_real.cpp)
if(UNIX)
    add_qamui_test(test_terminal_device test_terminal_device.cpp)
    set_tests_properties(test_terminal_device PROPERTIES TIMEOUT 20)

    # A live process under a pseudo-terminal: what the real descriptor receives
    # on Ctrl+Z, fg and SIGTERM, and what the tty's modes are afterwards.
    add_executable(qamui_signal_fixture signal_fixture.cpp)
    target_link_libraries(qamui_signal_fixture PRIVATE Qt6::Core Qamui::Core)
    add_qamui_test(test_signal_safety test_signal_safety.cpp)
    target_compile_definitions(test_signal_safety PRIVATE
        QAMUI_SIGNAL_FIXTURE="$<TARGET_FILE:qamui_signal_fixture>")
    add_dependencies(test_signal_safety qamui_signal_fixture)
    if(NOT APPLE)
        target_link_libraries(test_signal_safety PRIVATE util)   # forkpty
    endif()
endif()

add_qamui_test(test_terminal_framework test_terminal_framework.cpp)
add_qamui_test(test_mouse_modes test_mouse_modes.cpp)
target_link_libraries(test_mouse_modes PRIVATE Qamui::Runner)
add_qamui_test(test_render_loop test_render_loop.cpp)
add_qamui_test(test_input_router test_input_router.cpp)
add_qamui_test(test_focus test_focus.cpp)
if(QAMUI_ENABLE_BEHAVIOR)
    add_qamui_test(test_behavior test_behavior.cpp)
endif()

add_qamui_test(test_layout test_layout.cpp)
add_qamui_test(test_text_layout test_text_layout.cpp)
add_qamui_test(test_text_input test_text_input.cpp)
add_qamui_test(test_canvas test_canvas.cpp)
add_qamui_test(test_repeater test_repeater.cpp)
add_qamui_test(test_loader test_loader.cpp)
add_qamui_test(test_scroll_view test_scroll_view.cpp)
add_qamui_test(test_views_virtualization test_views_virtualization.cpp)
add_qamui_test(test_table_view_hardening test_table_view_hardening.cpp)
add_qamui_test(test_model_contracts test_model_contracts.cpp)
add_qamui_test(test_tree_view test_tree_view.cpp)

if(QAMUI_BUILD_BENCHMARKS)
    add_executable(benchmark_table_view benchmark_table_view.cpp)
    target_link_libraries(benchmark_table_view PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

    # Per-phase cost of each model view (delegate creation, scroll, data
    # change, idle frame) and memory per delegate. See docs/benchmarks/.
    add_executable(benchmark_view_phases benchmark_view_phases.cpp)
    target_link_libraries(benchmark_view_phases PRIVATE
        Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

    add_executable(benchmark_views benchmark_views.cpp)
    target_link_libraries(benchmark_views PRIVATE
        Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

    # Per-frame paint/diff cost for representative scenes. Informational; run
    # it before and after rendering changes and record the numbers.
    add_executable(benchmark_render benchmark_render.cpp)
    target_link_libraries(benchmark_render PRIVATE
        Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
    add_test(NAME test_benchmark_render_smoke COMMAND benchmark_render --strict --frames 2)
    set_tests_properties(test_benchmark_render_smoke PROPERTIES
        ENVIRONMENT "QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
    )
endif()

add_qamui_test(test_span test_span.cpp)
add_qamui_test(test_rectangle test_rectangle.cpp)
add_executable(qamui_span_fixture span_fixture.cpp)
target_link_libraries(qamui_span_fixture PRIVATE Qamui::Core Qamui::Runner)
qamui_add_scenario(
    NAME test_span_scenario
    TARGET qamui_span_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/span_scenario.js"
    COLUMNS 30 ROWS 5
    LABELS AgentSmoke
)

add_executable(qamui_table_grid_fixture table_grid_fixture.cpp)
target_link_libraries(qamui_table_grid_fixture PRIVATE Qamui::Core Qamui::Runner)
qamui_add_scenario(
    NAME tableview_box_grid
    TARGET qamui_table_grid_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/table_grid_scenario.js"
    COLUMNS 164 ROWS 25
    LABELS AgentSmoke
)

add_executable(qamui_process_table_fixture process_table_fixture.cpp)
target_link_libraries(qamui_process_table_fixture PRIVATE Qamui::Core Qamui::Runner)
qamui_add_scenario(
    NAME process_table_scenario
    TARGET qamui_process_table_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/process_table_scenario.js"
    COLUMNS 80 ROWS 24
    LABELS AgentSmoke
)
