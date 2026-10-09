# One directory per exported library. Every module includes Qtui headers as
# <Qtui/<Module>/...> from the single include root src/, and installs them to
# the same path under ${CMAKE_INSTALL_INCLUDEDIR}; qtui_install_headers()
# records what is public so qtui_check_public_headers() can prove that public
# headers only reach other public headers.
include("${Qtui_SOURCE_DIR}/cmake/QtuiPublicHeaders.cmake")

add_subdirectory(Core)
if(QTUI_BUILD_TESTS OR QTUI_ENABLE_TEST_RUNNER)
    add_subdirectory(TestRunner)
endif()
add_subdirectory(Controls)
if(QTUI_ENABLE_IMAGE)
    add_subdirectory(Image)
endif()
if(QTUI_ENABLE_TERMINAL_HOST)
    add_subdirectory(TerminalHost)
endif()

qtui_check_public_headers()
