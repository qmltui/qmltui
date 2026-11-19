# Tests for qmltui::core: Primitives, Engine, Items, and the benchmarks.

if(QMLTUI_BUILD_TESTS)
add_qmltui_test(test_clipboard test_clipboard.cpp)
target_link_libraries(test_clipboard PRIVATE qmltui::runner)

add_qmltui_test(test_input_decoder test_input_decoder.cpp)
add_qmltui_test(test_text_grid_buffer test_text_grid_buffer.cpp)
add_qmltui_test(test_unicode_text test_unicode_text.cpp)
add_qmltui_test(test_subpixel test_subpixel.cpp)
add_qmltui_test(test_color_depth test_color_depth.cpp)
add_qmltui_test(test_color_type_resolution test_color_type_resolution.cpp)
add_qmltui_test(test_terminal_capabilities test_terminal_capabilities.cpp)
add_qmltui_test(test_terminal_backend test_terminal_backend.cpp)
target_link_libraries(test_terminal_backend PRIVATE qmltui::runner)
add_qmltui_test(test_terminal_queries test_terminal_queries.cpp)
target_link_libraries(test_terminal_queries PRIVATE qmltui::runner)
add_qmltui_test(test_clock_virtual test_clock_virtual.cpp)
add_qmltui_test(test_clock_real test_clock_real.cpp)
if(UNIX AND NOT EMSCRIPTEN)
    add_qmltui_test(test_terminal_device test_terminal_device.cpp)
    set_tests_properties(test_terminal_device PROPERTIES TIMEOUT 20)

    # A live process under a pseudo-terminal: what the real descriptor receives
    # on Ctrl+Z, fg and SIGTERM, and what the tty's modes are afterwards.
    add_executable(qmltui_signal_fixture signal_fixture.cpp)
    target_link_libraries(qmltui_signal_fixture PRIVATE Qt6::Core qmltui::core)
    add_qmltui_test(test_signal_safety test_signal_safety.cpp)
    set_tests_properties(test_signal_safety PROPERTIES TIMEOUT 20)
    target_compile_definitions(test_signal_safety PRIVATE
        QMLTUI_SIGNAL_FIXTURE="$<TARGET_FILE:qmltui_signal_fixture>")
    add_dependencies(test_signal_safety qmltui_signal_fixture)
    if(NOT APPLE)
        target_link_libraries(test_signal_safety PRIVATE util)   # forkpty
    endif()
endif()

add_qmltui_test(test_terminal_framework test_terminal_framework.cpp)
add_qmltui_test(test_qt_message_routing test_qt_message_routing.cpp)
add_qmltui_test(test_mouse_modes test_mouse_modes.cpp)
target_link_libraries(test_mouse_modes PRIVATE qmltui::runner)
add_qmltui_test(test_render_loop test_render_loop.cpp)
add_qmltui_test(test_input_router test_input_router.cpp)
add_qmltui_test(test_focus test_focus.cpp)
if(QMLTUI_ENABLE_PRIVATE_QT)
    add_qmltui_test(test_behavior test_behavior.cpp)
endif()
add_qmltui_test(test_states test_states.cpp)
if(QMLTUI_ENABLE_PRIVATE_QT)
    target_compile_definitions(test_states PRIVATE QMLTUI_ENABLE_PRIVATE_QT)
endif()

add_qmltui_test(test_layout test_layout.cpp)
add_qmltui_test(test_text_layout test_text_layout.cpp)
add_qmltui_test(test_text_input test_text_input.cpp)
add_qmltui_test(test_text_area_highlighting test_text_area_highlighting.cpp)
add_qmltui_test(test_canvas test_canvas.cpp)
add_qmltui_test(test_repeater test_repeater.cpp)
add_qmltui_test(test_loader test_loader.cpp)
add_qmltui_test(test_scroll_view test_scroll_view.cpp)
add_qmltui_test(test_views_virtualization test_views_virtualization.cpp)
add_qmltui_test(test_table_view_hardening test_table_view_hardening.cpp)
add_qmltui_test(test_model_contracts test_model_contracts.cpp)
add_qmltui_test(test_tree_view test_tree_view.cpp)
endif()

if(QMLTUI_BUILD_BENCHMARKS)
    add_executable(benchmark_table_view benchmark_table_view.cpp)
    target_link_libraries(benchmark_table_view PRIVATE Qt6::Core Qt6::Qml qmltui::core)

    # Per-phase cost of each model view (delegate creation, scroll, data
    # change, idle frame) and memory per delegate. See docs/contributing/benchmarks/.
    add_executable(benchmark_view_phases benchmark_view_phases.cpp)
    target_link_libraries(benchmark_view_phases PRIVATE
        Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

    add_executable(benchmark_views benchmark_views.cpp)
    target_link_libraries(benchmark_views PRIVATE
        Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

    # Side-by-side model/view comparison against Qt Quick and Qt Widgets when
    # those optional Qt modules are installed on the host.
    find_package(Qt6 QUIET OPTIONAL_COMPONENTS Widgets Quick)
    add_executable(benchmark_view_compare benchmark_view_compare.cpp)
    target_link_libraries(benchmark_view_compare PRIVATE
        Qt6::Core Qt6::Gui Qt6::Qml qmltui::core qmltui::controls)
    if(TARGET Qt6::Widgets)
        target_link_libraries(benchmark_view_compare PRIVATE Qt6::Widgets)
        target_compile_definitions(benchmark_view_compare PRIVATE QMLTUI_BENCHMARK_HAS_WIDGETS=1)
    endif()
    if(TARGET Qt6::Quick)
        target_link_libraries(benchmark_view_compare PRIVATE Qt6::Quick)
        target_compile_definitions(benchmark_view_compare PRIVATE QMLTUI_BENCHMARK_HAS_QUICK=1)
    endif()
    if(QMLTUI_BUILD_TESTS)
        add_test(NAME test_benchmark_view_compare_smoke
            COMMAND benchmark_view_compare --rows 1000 --steps 10)
        set_tests_properties(test_benchmark_view_compare_smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
        )
    endif()

    # Per-frame paint/diff cost for representative scenes. Informational; run
    # it before and after rendering changes and record the numbers.
    add_executable(benchmark_render benchmark_render.cpp)
    target_link_libraries(benchmark_render PRIVATE
        Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
    if(QMLTUI_BUILD_TESTS)
        add_test(NAME test_benchmark_render_smoke COMMAND benchmark_render --strict --frames 2)
        set_tests_properties(test_benchmark_render_smoke PROPERTIES
            ENVIRONMENT "QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
        )
    endif()
endif()

if(QMLTUI_BUILD_TESTS)
add_qmltui_test(test_span test_span.cpp)
add_qmltui_test(test_rectangle test_rectangle.cpp)
add_executable(qmltui_span_fixture span_fixture.cpp)
target_link_libraries(qmltui_span_fixture PRIVATE qmltui::core qmltui::runner)
qmltui_add_scenario(
    NAME test_span_scenario
    TARGET qmltui_span_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/span_scenario.js"
    COLUMNS 30 ROWS 5
)

add_executable(qmltui_table_grid_fixture table_grid_fixture.cpp)
target_link_libraries(qmltui_table_grid_fixture PRIVATE qmltui::core qmltui::runner)
qmltui_add_scenario(
    NAME tableview_box_grid
    TARGET qmltui_table_grid_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/table_grid_scenario.js"
    COLUMNS 164 ROWS 25
)

add_executable(qmltui_process_table_fixture process_table_fixture.cpp)
target_link_libraries(qmltui_process_table_fixture PRIVATE qmltui::core qmltui::runner)
qmltui_add_scenario(
    NAME process_table_scenario
    TARGET qmltui_process_table_fixture
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/process_table_scenario.js"
    COLUMNS 80 ROWS 24
)
endif()
