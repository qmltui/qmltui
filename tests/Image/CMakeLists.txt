# Tests for qmltui::image.

add_qmltui_test(test_image test_image.cpp)
add_qmltui_test(test_animated_sprite test_animated_sprite.cpp)
target_link_libraries(test_image PRIVATE Qt6::Network)   # the remote-source test serves a picture over a local socket
add_qmltui_test(test_kitty_graphics test_kitty_graphics.cpp)
target_link_libraries(test_kitty_graphics PRIVATE qmltui::runner)
