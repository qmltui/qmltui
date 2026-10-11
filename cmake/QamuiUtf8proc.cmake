# Provides the `utf8proc` target Qamui::Core links for Unicode width and
# grapheme handling: the system library when QAMUI_USE_SYSTEM_UTF8PROC is on
# (CMake config first, pkg-config second), otherwise a pinned FetchContent
# checkout built as part of this tree.

function(qamui_provide_utf8proc)
    if(TARGET utf8proc)
        return()
    endif()
    if(QAMUI_USE_SYSTEM_UTF8PROC)
        find_package(utf8proc CONFIG QUIET)
        if(TARGET utf8proc::utf8proc)
            add_library(utf8proc ALIAS utf8proc::utf8proc)
            return()
        endif()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(utf8proc REQUIRED IMPORTED_TARGET libutf8proc)
        add_library(utf8proc ALIAS PkgConfig::utf8proc)
        return()
    endif()

    include(FetchContent)
    set(UTF8PROC_INSTALL OFF CACHE BOOL "" FORCE)
    set(UTF8PROC_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        utf8proc
        GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc.git
        GIT_TAG v2.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(utf8proc)
    # utf8proc is plain C. A consumer that ran qt_standard_project_setup() before
    # fetching Qamui has CMAKE_AUTOMOC on, which would add a C++ moc stub to this
    # target and compile it with utf8proc's -std=c99 (clang rejects that).
    set_target_properties(utf8proc PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
endfunction()
