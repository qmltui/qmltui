set(TESTS
    test_input_decoder
    test_text_grid_buffer
    test_layout
    test_focus
    test_text_input
    test_scroll_view
    test_popup
    test_flex_layout
)

include(CTest)
enable_testing()

foreach(test_name ${TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE Qt6::Core Qt6::Test Qtui)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
