cmake_minimum_required(VERSION 3.20)
project(qmltui VERSION 0.5.0 LANGUAGES CXX)

option(QMLTUI_BUILD_EXAMPLES   "Build the examples" ${PROJECT_IS_TOP_LEVEL})
option(QMLTUI_BUILD_TESTS      "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(QMLTUI_BUILD_BENCHMARKS "Build performance benchmark executables" ${PROJECT_IS_TOP_LEVEL})
option(QMLTUI_ENABLE_PRIVATE_QT "Enable features requiring private Qt modules (Behavior, advanced states)" ON)
option(QMLTUI_ENABLE_IMAGE     "Build the optional QmlTui Image module (requires Qt Gui and Network)" OFF)
option(QMLTUI_ENABLE_PTYVIEW  "Build the optional Unix PTY PtyView module" OFF)
option(QMLTUI_ENABLE_RUNNER "Build and install the optional QmlTui Runner library and CLI" ${PROJECT_IS_TOP_LEVEL})
option(QMLTUI_ENABLE_COVERAGE "Instrument QmlTui targets for Clang source-based code coverage" OFF)
option(QMLTUI_COMMERCIAL_LICENSE "Acknowledge that this build is covered by a valid QmlTui Commercial License" OFF)

if(QMLTUI_COMMERCIAL_LICENSE)
    message(STATUS "QmlTui: Building under Commercial License terms.")
else()
    message(STATUS "QmlTui: Building under LGPLv3 terms.")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(WIN32)
    # Public classes carry QMLTUI_*_EXPORT (see primitives/QmlTuiExport.h); the
    # export-all pass additionally covers free functions and helpers that other
    # QmlTui DLLs reach. Symbols declared both ways trigger LNK4197, which is
    # harmless and silenced here to keep build logs readable.
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if(MSVC)
        add_link_options(/ignore:4197)
    elseif(MINGW)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--export-all-symbols")
    endif()
endif()
set(QMLTUI_INSTALL_QMLDIR "${CMAKE_INSTALL_LIBDIR}/qml" CACHE PATH
    "Install location for QmlTui QML modules")
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH
    "Build-tree location for generated QML modules")
file(MAKE_DIRECTORY "${QT_QML_OUTPUT_DIRECTORY}")
set(_qmltui_qml_import_paths ${QML_IMPORT_PATH})
if(NOT "${QT_QML_OUTPUT_DIRECTORY}" IN_LIST _qmltui_qml_import_paths)
    list(APPEND _qmltui_qml_import_paths "${QT_QML_OUTPUT_DIRECTORY}")
    set(QML_IMPORT_PATH "${_qmltui_qml_import_paths}" CACHE STRING
        "QML import paths for Qt Creator and QML tooling" FORCE)
endif()
unset(_qmltui_qml_import_paths)

# Third-party code: each option picks a system library or the pinned in-tree
# copy; the mechanics live in cmake/QmlTuiUtf8proc.cmake and cmake/QmlTuiLibvterm.cmake.
option(QMLTUI_USE_SYSTEM_UTF8PROC "Use system utf8proc library" OFF)
option(QMLTUI_USE_SYSTEM_LIBVTERM  "Use system libvterm library (PtyView only)" OFF)
include(cmake/QmlTuiUtf8proc.cmake)
include(cmake/QmlTuiLibvterm.cmake)
qmltui_provide_utf8proc()   # qmltui::core links it; PtyView provides libvterm itself

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(qt_required_components Core Qml)

if(QMLTUI_ENABLE_IMAGE)
    list(APPEND qt_required_components Gui Network)   # Network: QmlTui.Image fetches http(s) sources; Gui: QImage
endif()

if(QMLTUI_BUILD_TESTS)
    list(APPEND qt_required_components Test Gui)   # Gui: QStandardItemModel in TableView/TreeView unit tests
endif()
list(REMOVE_DUPLICATES qt_required_components)

if(QMLTUI_ENABLE_PRIVATE_QT)
    set(QT_NO_PRIVATE_MODULE_WARNING ON CACHE BOOL
        "Suppress warnings about the optional Qt QmlPrivate dependency")
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS ${qt_required_components})

if(NOT QT6_IS_SHARED_LIBS_BUILD OR (DEFINED BUILD_SHARED_LIBS AND NOT BUILD_SHARED_LIBS))
    set(QMLTUI_STATIC_QML_MODULE STATIC)
else()
    set(QMLTUI_STATIC_QML_MODULE)
endif()

if(QMLTUI_ENABLE_PRIVATE_QT)
    if(NOT TARGET Qt6::QmlPrivate)
        find_package(Qt6 6.8 QUIET OPTIONAL_COMPONENTS QmlPrivate)
    endif()
    if(NOT TARGET Qt6::QmlPrivate)
        message(WARNING
            "QmlTui: Qt6::QmlPrivate was not found; building without Behavior and advanced states. "
            "Install the Qt 6 Qml private development headers to enable them, or pass "
            "-DQMLTUI_ENABLE_PRIVATE_QT=OFF to suppress this warning.")
        set(QMLTUI_ENABLE_PRIVATE_QT OFF)
        if(NOT PROJECT_IS_TOP_LEVEL)
            set(QMLTUI_ENABLE_PRIVATE_QT OFF PARENT_SCOPE)
        endif()
    endif()
endif()

qt_standard_project_setup(REQUIRES 6.8)

if(EMSCRIPTEN)
    include(cmake/QmlTuiWasm.cmake)
endif()

if(QMLTUI_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

add_subdirectory(src/QmlTui)
if(TARGET qmltui::runner AND NOT EMSCRIPTEN)
    add_subdirectory(tools/qmltui-runner)
endif()

if(QMLTUI_ENABLE_COVERAGE)
    include(cmake/QmlTuiCoverage.cmake)
    qmltui_register_coverage_targets()
endif()

# Example executable to test the library
if(QMLTUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(QMLTUI_BUILD_TESTS)
    add_subdirectory(tests)
endif()

configure_package_config_file(
    cmake/QmlTuiConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/QmlTuiConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QmlTui"
    PATH_VARS QMLTUI_INSTALL_QMLDIR
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QmlTuiConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion
)
install(EXPORT QmlTuiTargets
    FILE QmlTuiTargets.cmake
    NAMESPACE qmltui::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QmlTui"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QmlTuiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QmlTuiConfigVersion.cmake"
    cmake/QmlTuiRunner.cmake
    cmake/RunQmlTuiScenario.cmake
    cmake/QmlTuiWasm.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QmlTui"
)
