cmake_minimum_required(VERSION 3.20)
project(Qtui VERSION 0.4.0 LANGUAGES CXX)

option(QTUI_BUILD_EXAMPLES   "Build the examples" ${PROJECT_IS_TOP_LEVEL})
option(QTUI_BUILD_TESTS      "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(QTUI_BUILD_BENCHMARKS "Build performance benchmark executables" ${PROJECT_IS_TOP_LEVEL})
option(QTUI_ENABLE_BEHAVIOR  "Enable Behavior on properties (requires Qt QmlPrivate)" OFF)
option(QTUI_ENABLE_TERMINAL_HOST "Build the optional Unix PTY TerminalView module" OFF)
option(QTUI_ENABLE_TEST_RUNNER "Build and install the optional Qtui TestRunner library" OFF)
option(QTUI_COMMERCIAL_LICENSE "Acknowledge that this build is covered by a valid Qtui Commercial License" OFF)

if(QTUI_COMMERCIAL_LICENSE)
    message(STATUS "Qtui: Building under Commercial License terms.")
    add_compile_definitions(QTUI_IS_COMMERCIAL=1)
else()
    message(STATUS "Qtui: Building under LGPLv3 / GPLv3 terms.")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(WIN32)
    # Public classes carry QTUI_*_EXPORT (see primitives/QtuiExport.h); the
    # export-all pass additionally covers free functions and helpers that other
    # Qtui DLLs reach. Symbols declared both ways trigger LNK4197, which is
    # harmless and silenced here to keep build logs readable.
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if(MSVC)
        add_link_options(/ignore:4197)
    endif()
endif()
set(QTUI_INSTALL_QMLDIR "${CMAKE_INSTALL_LIBDIR}/qml" CACHE PATH
    "Install location for Qtui QML modules")
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH
    "Build-tree location for generated QML modules")

# Third-party code: each option picks a system library or the pinned in-tree
# copy; the mechanics live in cmake/QtuiUtf8proc.cmake and cmake/QtuiLibvterm.cmake.
option(QTUI_USE_SYSTEM_UTF8PROC "Use system utf8proc library" OFF)
option(QTUI_USE_SYSTEM_LIBVTERM  "Use system libvterm library (TerminalHost only)" OFF)
include(cmake/QtuiUtf8proc.cmake)
include(cmake/QtuiLibvterm.cmake)
qtui_provide_utf8proc()   # Qtui::Core links it; TerminalHost provides libvterm itself

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(qt_required_components Core Qml Gui)

if(QTUI_BUILD_TESTS)
    list(APPEND qt_required_components Test)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS ${qt_required_components})

if(QTUI_ENABLE_BEHAVIOR)
    set(QT_NO_PRIVATE_MODULE_WARNING ON CACHE BOOL
        "Suppress warnings about the optional Qt QmlPrivate dependency")
    if(NOT TARGET Qt6::QmlPrivate)
        find_package(Qt6 6.8 OPTIONAL_COMPONENTS QmlPrivate)
    endif()
endif()

qt_standard_project_setup(REQUIRES 6.8)

if(QTUI_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

add_subdirectory(src/Qtui)

# Example executable to test the library
if(QTUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
elseif(QTUI_BUILD_TESTS)
    if(QTUI_ENABLE_BEHAVIOR)
        add_subdirectory(examples/apps/industrial_plc_hmi
            examples/apps/industrial_plc_hmi)
    endif()
    add_subdirectory(examples/apps/pos_cashier
        examples/apps/pos_cashier)
endif()

if(QTUI_BUILD_TESTS)
    add_subdirectory(tests)
endif()

configure_package_config_file(
    cmake/QtuiConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/QtuiConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qtui"
    PATH_VARS QTUI_INSTALL_QMLDIR
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QtuiConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion
)
install(EXPORT QtuiTargets
    FILE QtuiTargets.cmake
    NAMESPACE Qtui::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qtui"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QtuiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QtuiConfigVersion.cmake"
    cmake/QtuiTestRunner.cmake
    cmake/RunQtuiScenario.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qtui"
)
