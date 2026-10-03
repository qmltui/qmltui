include(CTest)
enable_testing()

function(add_qtui_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE Qt6::Core Qt6::Test Qtui::Core Qtui::Controls Qtui::Extras Qtui::Canvas)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()


add_qtui_test(test_input_decoder Core/test_input_decoder.cpp)
add_qtui_test(test_text_grid_buffer Core/test_text_grid_buffer.cpp)
add_qtui_test(test_layout Core/test_layout.cpp)
add_qtui_test(test_loader Core/test_loader.cpp)
add_qtui_test(test_focus Core/test_focus.cpp)
add_qtui_test(test_text_input Core/test_text_input.cpp)
add_qtui_test(test_scroll_view Core/test_scroll_view.cpp)
add_qtui_test(test_views_virtualization Core/test_views_virtualization.cpp)
add_qtui_test(test_terminal_framework Core/test_terminal_framework.cpp)

add_qtui_test(test_canvas Canvas/test_canvas.cpp)
add_qtui_test(test_popup Controls/test_popup.cpp)
add_qtui_test(test_extras Extras/test_extras.cpp)

if(QTUI_BUILD_TERMINAL_HOST)
    add_executable(test_terminal_host TerminalHost/test_terminal_host.cpp)
    target_link_libraries(test_terminal_host PRIVATE
        Qt6::Core Qt6::Gui Qt6::Test Qt6::Qml Qtui::TerminalHost qtui_libvterm
    )
    target_compile_definitions(test_terminal_host PRIVATE
        QTUI_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
    )
    add_test(NAME test_terminal_host COMMAND test_terminal_host)
endif()

if(QTUI_BUILD_BENCHMARKS)
    add_executable(benchmark_views Core/benchmark_views.cpp)
    target_link_libraries(benchmark_views PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::Extras Qtui::Canvas)
endif()
