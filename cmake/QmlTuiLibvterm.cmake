# Provides the `qmltui_libvterm` target the optional qmltui::pty_view module
# links for its embedded terminal emulator: the system libvterm through
# pkg-config when QMLTUI_USE_SYSTEM_LIBVTERM is on, otherwise the pinned 0.3.3
# release tarball compiled as a static library inside this tree. PtyView
# is the only consumer, so its CMakeLists.txt is the only caller.
#
# A macro, not a function: enable_language() must run at directory scope.

macro(qmltui_provide_libvterm)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
        message(FATAL_ERROR "QMLTUI_ENABLE_PTY_VIEW currently supports only Linux and macOS")
    endif()
    if(TARGET qmltui_libvterm)
        # already provided
    elseif(QMLTUI_USE_SYSTEM_LIBVTERM)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(libvterm REQUIRED IMPORTED_TARGET vterm)
        add_library(qmltui_libvterm ALIAS PkgConfig::libvterm)
    else()
        enable_language(C)
        _qmltui_fetch_libvterm()
    endif()
endmacro()

function(_qmltui_fetch_libvterm)
    include(FetchContent)
    FetchContent_Declare(libvterm
        URL https://www.leonerd.org.uk/code/libvterm/libvterm-0.3.3.tar.gz
        URL_HASH SHA256=09156f43dd2128bd347cbeebe50d9a571d32c64e0cf18d211197946aff7226e0
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(libvterm)

    # The tarball has no CMake project of its own; list its sources here.
    add_library(qmltui_libvterm STATIC
        ${libvterm_SOURCE_DIR}/src/encoding.c
        ${libvterm_SOURCE_DIR}/src/keyboard.c
        ${libvterm_SOURCE_DIR}/src/mouse.c
        ${libvterm_SOURCE_DIR}/src/parser.c
        ${libvterm_SOURCE_DIR}/src/pen.c
        ${libvterm_SOURCE_DIR}/src/screen.c
        ${libvterm_SOURCE_DIR}/src/state.c
        ${libvterm_SOURCE_DIR}/src/unicode.c
        ${libvterm_SOURCE_DIR}/src/vterm.c
    )
    target_compile_features(qmltui_libvterm PRIVATE c_std_99)
    target_include_directories(qmltui_libvterm PUBLIC $<BUILD_INTERFACE:${libvterm_SOURCE_DIR}/include>)
    set_target_properties(qmltui_libvterm PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
    )
endfunction()
