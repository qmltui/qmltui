# Provides the `utf8proc` target QmlTui::Core links for Unicode width and
# grapheme handling: the system library when QMLTUI_USE_SYSTEM_UTF8PROC is on
# (CMake config first, pkg-config second), otherwise a pinned FetchContent
# checkout built as part of this tree.

function(qmltui_provide_utf8proc)
    if(TARGET utf8proc)
        return()
    endif()
    if(QMLTUI_USE_SYSTEM_UTF8PROC)
        find_package(utf8proc CONFIG QUIET)
        if(TARGET utf8proc::utf8proc)
            set(QMLTUI_UTF8PROC_BACKEND "CONFIG" CACHE INTERNAL "utf8proc target backend" FORCE)
            add_library(utf8proc ALIAS utf8proc::utf8proc)
            return()
        endif()
        find_package(PkgConfig REQUIRED)
        if(NOT TARGET PkgConfig::utf8proc)
            pkg_check_modules(utf8proc REQUIRED IMPORTED_TARGET GLOBAL libutf8proc)
        endif()
        set(QMLTUI_UTF8PROC_BACKEND "PKGCONFIG" CACHE INTERNAL "utf8proc target backend" FORCE)
        add_library(utf8proc ALIAS PkgConfig::utf8proc)
        return()
    endif()

    set(QMLTUI_UTF8PROC_BACKEND "BUNDLED" CACHE INTERNAL "utf8proc target backend" FORCE)
    include(FetchContent)
    set(UTF8PROC_INSTALL OFF CACHE BOOL "" FORCE)
    set(UTF8PROC_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    FetchContent_Declare(
        utf8proc
        URL https://github.com/JuliaStrings/utf8proc/archive/refs/tags/v2.11.3.tar.gz
        URL_HASH SHA256=abfed50b6d4da51345713661370290f4f4747263ee73dc90356299dfc7990c78
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(utf8proc)
    # utf8proc is plain C. A consumer that ran qt_standard_project_setup() before
    # fetching QmlTui has CMAKE_AUTOMOC on, which would add a C++ moc stub to this
    # target and compile it with utf8proc's -std=c99 (clang rejects that).
    # Prefix OUTPUT_NAME so bundled static installs do not collide with system libutf8proc.a.
    set_target_properties(utf8proc PROPERTIES
        OUTPUT_NAME qmltui_utf8proc
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
    )
endfunction()
