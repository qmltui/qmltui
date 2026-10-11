# One directory per exported library. Every module includes Qamui headers as
# <Qamui/<Module>/...> from the single include root src/, and installs them to
# the same path under ${CMAKE_INSTALL_INCLUDEDIR}; qamui_install_headers()
# records what is public so qamui_check_public_headers() can prove that public
# headers only reach other public headers.
include("${Qamui_SOURCE_DIR}/cmake/QamuiPublicHeaders.cmake")

add_subdirectory(Core)
if(QAMUI_BUILD_TESTS OR QAMUI_ENABLE_RUNNER)
    add_subdirectory(Runner)
endif()
add_subdirectory(Controls)
if(QAMUI_ENABLE_IMAGE)
    add_subdirectory(Image)
endif()
if(QAMUI_ENABLE_TERMINAL_HOST)
    add_subdirectory(TerminalHost)
endif()

qamui_check_public_headers()
