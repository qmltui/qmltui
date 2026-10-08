# Tests for the optional Qtui::TerminalHost module (QTUI_ENABLE_TERMINAL_HOST).

add_executable(test_terminal_host test_terminal_host.cpp)
target_link_libraries(test_terminal_host PRIVATE
    Qt6::Core Qt6::Test Qt6::Qml Qtui::TerminalHost qtui_libvterm
)
target_compile_definitions(test_terminal_host PRIVATE
    QTUI_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
)
add_test(NAME test_terminal_host COMMAND test_terminal_host)
