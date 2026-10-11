# Provides the `qamui_libvterm` target the optional Qamui::TerminalHost module
# links for its embedded terminal emulator: the system libvterm through
# pkg-config when QAMUI_USE_SYSTEM_LIBVTERM is on, otherwise the pinned 0.3.3
# release tarball compiled as a static library inside this tree. TerminalHost
# is the only consumer, so its CMakeLists.txt is the only caller.
#
# A macro, not a function: enable_language() must run at directory scope.

macro(qamui_provide_libvterm)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
        message(FATAL_ERROR "QAMUI_ENABLE_TERMINAL_HOST currently supports only Linux and macOS")
    endif()
    if(TARGET qamui_libvterm)
        # already provided
    elseif(QAMUI_USE_SYSTEM_LIBVTERM)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(libvterm REQUIRED IMPORTED_TARGET vterm)
        add_library(qamui_libvterm ALIAS PkgConfig::libvterm)
    else()
        enable_language(C)
        _qamui_fetch_libvterm()
    endif()
endmacro()

function(_qamui_fetch_libvterm)
    include(FetchContent)
    FetchContent_Declare(libvterm
        URL https://www.leonerd.org.uk/code/libvterm/libvterm-0.3.3.tar.gz
        URL_HASH SHA256=09156f43dd2128bd347cbeebe50d9a571d32c64e0cf18d211197946aff7226e0
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(libvterm)

    # The tarball has no CMake project of its own; list its sources here.
    add_library(qamui_libvterm STATIC
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
    target_compile_features(qamui_libvterm PRIVATE c_std_99)
    target_include_directories(qamui_libvterm PUBLIC ${libvterm_SOURCE_DIR}/include)
    set_target_properties(qamui_libvterm PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
    )
endfunction()
