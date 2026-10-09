# Tests for Qtui::Image.

add_qtui_test(test_image test_image.cpp)
target_link_libraries(test_image PRIVATE Qt6::Network)   # the remote-source test serves a picture over a local socket
add_qtui_test(test_kitty_graphics test_kitty_graphics.cpp)
target_link_libraries(test_kitty_graphics PRIVATE Qtui::TestRunner)
