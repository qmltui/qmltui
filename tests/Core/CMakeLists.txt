# Tests for Qtui::Core: primitives, engine, items, and the benchmarks.

add_qtui_test(test_input_decoder test_input_decoder.cpp)
add_qtui_test(test_text_grid_buffer test_text_grid_buffer.cpp)
add_qtui_test(test_unicode_text test_unicode_text.cpp)
add_qtui_test(test_color_depth test_color_depth.cpp)
add_qtui_test(test_terminal_capabilities test_terminal_capabilities.cpp)
add_qtui_test(test_terminal_backend test_terminal_backend.cpp)
target_link_libraries(test_terminal_backend PRIVATE Qtui::TestRunner)
add_qtui_test(test_terminal_queries test_terminal_queries.cpp)
target_link_libraries(test_terminal_queries PRIVATE Qtui::TestRunner)
if(UNIX)
    add_qtui_test(test_terminal_device test_terminal_device.cpp)
    set_tests_properties(test_terminal_device PROPERTIES TIMEOUT 20)

    # A live process under a pseudo-terminal: what the real descriptor receives
    # on Ctrl+Z, fg and SIGTERM, and what the tty's modes are afterwards.
    add_executable(qtui_signal_fixture signal_fixture.cpp)
    target_link_libraries(qtui_signal_fixture PRIVATE Qt6::Core Qtui::Core)
    add_qtui_test(test_signal_safety test_signal_safety.cpp)
    target_compile_definitions(test_signal_safety PRIVATE
        QTUI_SIGNAL_FIXTURE="$<TARGET_FILE:qtui_signal_fixture>")
    add_dependencies(test_signal_safety qtui_signal_fixture)
    if(NOT APPLE)
        target_link_libraries(test_signal_safety PRIVATE util)   # forkpty
    endif()
endif()

add_qtui_test(test_terminal_framework test_terminal_framework.cpp)
add_qtui_test(test_mouse_modes test_mouse_modes.cpp)
target_link_libraries(test_mouse_modes PRIVATE Qtui::TestRunner)
add_qtui_test(test_render_loop test_render_loop.cpp)
add_qtui_test(test_input_router test_input_router.cpp)
add_qtui_test(test_focus test_focus.cpp)
if(QTUI_ENABLE_BEHAVIOR)
    add_qtui_test(test_behavior test_behavior.cpp)
endif()

add_qtui_test(test_layout test_layout.cpp)
add_qtui_test(test_text_layout test_text_layout.cpp)
add_qtui_test(test_text_input test_text_input.cpp)
add_qtui_test(test_canvas test_canvas.cpp)
add_qtui_test(test_repeater test_repeater.cpp)
add_qtui_test(test_loader test_loader.cpp)
add_qtui_test(test_scroll_view test_scroll_view.cpp)
add_qtui_test(test_views_virtualization test_views_virtualization.cpp)
add_qtui_test(test_table_view_hardening test_table_view_hardening.cpp)
add_qtui_test(test_model_contracts test_model_contracts.cpp)
add_qtui_test(test_tree_view test_tree_view.cpp)

if(QTUI_BUILD_BENCHMARKS)
    add_executable(benchmark_table_view benchmark_table_view.cpp)
    target_link_libraries(benchmark_table_view PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

    # Per-phase cost of each model view (delegate creation, scroll, data
    # change, idle frame) and memory per delegate. See docs/benchmarks/.
    add_executable(benchmark_view_phases benchmark_view_phases.cpp)
    target_link_libraries(benchmark_view_phases PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

    add_executable(benchmark_views benchmark_views.cpp)
    target_link_libraries(benchmark_views PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

    # Per-frame paint/diff cost for representative scenes. Informational; run
    # it before and after rendering changes and record the numbers.
    add_executable(benchmark_render benchmark_render.cpp)
    target_link_libraries(benchmark_render PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
    add_test(NAME test_benchmark_render_smoke COMMAND benchmark_render --strict --frames 2)
    set_tests_properties(test_benchmark_render_smoke PROPERTIES
        ENVIRONMENT "QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
    )
endif()
