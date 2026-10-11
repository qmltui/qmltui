cmake_minimum_required(VERSION 3.20)
project(Qamui VERSION 0.5.0 LANGUAGES CXX)

option(QAMUI_BUILD_EXAMPLES   "Build the examples" ${PROJECT_IS_TOP_LEVEL})
option(QAMUI_BUILD_TESTS      "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(QAMUI_BUILD_BENCHMARKS "Build performance benchmark executables" ${PROJECT_IS_TOP_LEVEL})
option(QAMUI_ENABLE_BEHAVIOR  "Enable Behavior on properties (requires Qt QmlPrivate)" OFF)
option(QAMUI_ENABLE_IMAGE     "Build the optional Qamui Image module (requires Qt Gui and Network)" ON)
option(QAMUI_ENABLE_TERMINAL_HOST "Build the optional Unix PTY TerminalView module" OFF)
option(QAMUI_ENABLE_RUNNER "Build and install the optional Qamui Runner library" OFF)
option(QAMUI_COMMERCIAL_LICENSE "Acknowledge that this build is covered by a valid Qamui Commercial License" OFF)

if(QAMUI_COMMERCIAL_LICENSE)
    message(STATUS "Qamui: Building under Commercial License terms.")
    add_compile_definitions(QAMUI_IS_COMMERCIAL=1)
else()
    message(STATUS "Qamui: Building under LGPLv3 / GPLv3 terms.")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(WIN32)
    # Public classes carry QAMUI_*_EXPORT (see primitives/QamuiExport.h); the
    # export-all pass additionally covers free functions and helpers that other
    # Qamui DLLs reach. Symbols declared both ways trigger LNK4197, which is
    # harmless and silenced here to keep build logs readable.
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if(MSVC)
        add_link_options(/ignore:4197)
    endif()
endif()
set(QAMUI_INSTALL_QMLDIR "${CMAKE_INSTALL_LIBDIR}/qml" CACHE PATH
    "Install location for Qamui QML modules")
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH
    "Build-tree location for generated QML modules")

# Third-party code: each option picks a system library or the pinned in-tree
# copy; the mechanics live in cmake/QamuiUtf8proc.cmake and cmake/QamuiLibvterm.cmake.
option(QAMUI_USE_SYSTEM_UTF8PROC "Use system utf8proc library" OFF)
option(QAMUI_USE_SYSTEM_LIBVTERM  "Use system libvterm library (TerminalHost only)" OFF)
include(cmake/QamuiUtf8proc.cmake)
include(cmake/QamuiLibvterm.cmake)
qamui_provide_utf8proc()   # Qamui::Core links it; TerminalHost provides libvterm itself

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(qt_required_components Core Qml)

if(QAMUI_ENABLE_IMAGE)
    list(APPEND qt_required_components Gui Network)   # Network: Qamui.Image fetches http(s) sources; Gui: QImage
endif()

if(QAMUI_BUILD_TESTS)
    list(APPEND qt_required_components Test Gui)   # Gui: QStandardItemModel in TableView/TreeView unit tests
endif()
list(REMOVE_DUPLICATES qt_required_components)

find_package(Qt6 6.8 REQUIRED COMPONENTS ${qt_required_components})

if(QAMUI_ENABLE_BEHAVIOR)
    set(QT_NO_PRIVATE_MODULE_WARNING ON CACHE BOOL
        "Suppress warnings about the optional Qt QmlPrivate dependency")
    if(NOT TARGET Qt6::QmlPrivate)
        find_package(Qt6 6.8 OPTIONAL_COMPONENTS QmlPrivate)
    endif()
endif()

qt_standard_project_setup(REQUIRES 6.8)

if(EMSCRIPTEN)
    include(cmake/QamuiWasm.cmake)
endif()

if(QAMUI_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

add_subdirectory(src/Qamui)

# Example executable to test the library
if(QAMUI_BUILD_EXAMPLES)
    add_subdirectory(examples)

endif()

if(QAMUI_BUILD_TESTS)
    add_subdirectory(tests)
endif()

configure_package_config_file(
    cmake/QamuiConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/QamuiConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qamui"
    PATH_VARS QAMUI_INSTALL_QMLDIR
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QamuiConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion
)
install(EXPORT QamuiTargets
    FILE QamuiTargets.cmake
    NAMESPACE Qamui::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qamui"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QamuiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QamuiConfigVersion.cmake"
    cmake/QamuiRunner.cmake
    cmake/RunQamuiScenario.cmake
    cmake/QamuiWasm.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qamui"
)
