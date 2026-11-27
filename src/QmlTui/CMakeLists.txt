# One directory per exported library. Every module includes QmlTui headers as
# <QmlTui/<Module>/...> from the single include root src/, and installs them to
# the same path under ${CMAKE_INSTALL_INCLUDEDIR}; qmltui_install_headers()
# records what is public so qmltui_check_public_headers() can prove that public
# headers only reach other public headers.
include("${qmltui_SOURCE_DIR}/cmake/QmlTuiPublicHeaders.cmake")

if(WIN32)
    # Public classes carry QMLTUI_*_EXPORT (see Primitives/QmlTuiExport.h); the
    # export-all pass additionally covers free functions and helpers that other
    # QmlTui DLLs reach. Symbols declared both ways trigger LNK4197, which is
    # harmless and silenced within src/QmlTui so third-party targets, tests, and
    # parent FetchContent consumers are unaffected.
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if(MSVC)
        add_link_options(/ignore:4197)
    elseif(MINGW)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--export-all-symbols")
    endif()
endif()

add_subdirectory(Core)
if(QMLTUI_BUILD_TESTS OR QMLTUI_ENABLE_RUNNER)
    add_subdirectory(Runner)
endif()
add_subdirectory(Controls)
if(QMLTUI_ENABLE_IMAGE)
    add_subdirectory(Image)
else()
    file(REMOVE_RECURSE "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/Image")
endif()

qmltui_check_public_headers()
