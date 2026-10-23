# Tests for the optional qmltui::pty_view module (QMLTUI_ENABLE_PTY_VIEW).

add_executable(test_pty_view test_pty_view.cpp)
target_link_libraries(test_pty_view PRIVATE
    Qt6::Core Qt6::Test Qt6::Qml qmltui::pty_view qmltui_libvterm
)
target_compile_definitions(test_pty_view PRIVATE
    QMLTUI_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
)
add_test(NAME test_pty_view COMMAND test_pty_view)
set_tests_properties(test_pty_view PROPERTIES
    LABELS Unit
)
