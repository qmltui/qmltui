# One directory per exported library. Every module includes QmlTui headers as
# <QmlTui/<Module>/...> from the single include root src/, and installs them to
# the same path under ${CMAKE_INSTALL_INCLUDEDIR}; qmltui_install_headers()
# records what is public so qmltui_check_public_headers() can prove that public
# headers only reach other public headers.
include("${qmltui_SOURCE_DIR}/cmake/QmlTuiPublicHeaders.cmake")

add_subdirectory(Core)
if(QMLTUI_BUILD_TESTS OR QMLTUI_ENABLE_RUNNER)
    add_subdirectory(Runner)
endif()
add_subdirectory(Controls)
if(QMLTUI_ENABLE_IMAGE)
    add_subdirectory(Image)
endif()
if(QMLTUI_ENABLE_PTYVIEW)
    add_subdirectory(PtyView)
endif()

qmltui_check_public_headers()
