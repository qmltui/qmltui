cmake_minimum_required(VERSION 3.21)
project(qmltui VERSION 0.5.0 LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_qmltui_desktop_top_level OFF)
if(PROJECT_IS_TOP_LEVEL AND NOT EMSCRIPTEN)
    set(_qmltui_desktop_top_level ON)
endif()
option(QMLTUI_BUILD_EXAMPLES   "Build the examples" ${_qmltui_desktop_top_level})
option(QMLTUI_BUILD_TESTS      "Build tests" ${_qmltui_desktop_top_level})
option(QMLTUI_BUILD_BENCHMARKS "Build performance benchmark executables" ${_qmltui_desktop_top_level})
unset(_qmltui_desktop_top_level)
option(QMLTUI_ENABLE_PRIVATE_QT "Enable features requiring private Qt modules (Behavior, advanced states)" ON)
option(QMLTUI_ENABLE_IMAGE     "Build the optional QmlTui Image module (requires Qt Gui and Network)" OFF)
option(QMLTUI_ENABLE_PTYVIEW  "Build the optional Unix PTY PtyView module" OFF)
set(_qmltui_runner_default OFF)
if((PROJECT_IS_TOP_LEVEL OR BUILD_TESTING) AND NOT EMSCRIPTEN)
    set(_qmltui_runner_default ON)
endif()
option(QMLTUI_ENABLE_RUNNER "Build and install the optional QmlTui Runner library and CLI" ${_qmltui_runner_default})
unset(_qmltui_runner_default)
option(QMLTUI_ENABLE_COVERAGE "Instrument QmlTui targets for Clang source-based code coverage" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(QMLTUI_INSTALL_QMLDIR "${CMAKE_INSTALL_LIBDIR}/qml" CACHE PATH
    "Install location for QmlTui QML modules")
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH
    "Build-tree location for generated QML modules")
file(MAKE_DIRECTORY "${QT_QML_OUTPUT_DIRECTORY}")
set(QmlTui_QML_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}")
set(_qmltui_qml_import_paths $CACHE{QML_IMPORT_PATH} ${QML_IMPORT_PATH})
list(REMOVE_DUPLICATES _qmltui_qml_import_paths)
if(NOT "${QT_QML_OUTPUT_DIRECTORY}" IN_LIST _qmltui_qml_import_paths)
    list(APPEND _qmltui_qml_import_paths "${QT_QML_OUTPUT_DIRECTORY}")
    set(QML_IMPORT_PATH "${_qmltui_qml_import_paths}" CACHE STRING
        "QML import paths for Qt Creator and QML tooling" FORCE)
endif()
unset(_qmltui_qml_import_paths)
if(NOT "${QmlTui_QML_IMPORT_PATH}" IN_LIST QT_QML_IMPORT_PATH)
    list(APPEND QT_QML_IMPORT_PATH "${QmlTui_QML_IMPORT_PATH}")
endif()
if(NOT PROJECT_IS_TOP_LEVEL)
    set(QmlTui_QML_IMPORT_PATH "${QmlTui_QML_IMPORT_PATH}" PARENT_SCOPE)
    set(QT_QML_IMPORT_PATH "${QT_QML_IMPORT_PATH}" PARENT_SCOPE)
endif()

# Third-party code: each option picks a system library or the pinned source
# fetched via FetchContent; the mechanics live in cmake/QmlTuiUtf8proc.cmake and cmake/QmlTuiLibvterm.cmake.
option(QMLTUI_USE_SYSTEM_UTF8PROC "Use system utf8proc library" OFF)
option(QMLTUI_USE_SYSTEM_LIBVTERM  "Use system libvterm library (PtyView only)" OFF)
include(cmake/QmlTuiUtf8proc.cmake)
include(cmake/QmlTuiLibvterm.cmake)
qmltui_provide_utf8proc()   # qmltui::core links it; PtyView provides libvterm itself

set(qt_required_components Core Qml)

if(QMLTUI_BUILD_TESTS)
    list(APPEND qt_required_components Test Gui)   # Gui: QStandardItemModel in TableView/TreeView unit tests
elseif(QMLTUI_BUILD_BENCHMARKS)
    list(APPEND qt_required_components Gui)        # Gui: benchmark_view_compare
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

if(QMLTUI_ENABLE_IMAGE)
    # Network: QmlTui.Image fetches http(s) sources; Gui: QImage
    if(NOT TARGET Qt6::Gui OR NOT TARGET Qt6::Network)
        find_package(Qt6 6.8 QUIET OPTIONAL_COMPONENTS Gui Network)
    endif()
    if(NOT TARGET Qt6::Gui OR NOT TARGET Qt6::Network)
        message(WARNING
            "QmlTui: Qt6::Gui and/or Qt6::Network was not found; building without QmlTui.Image. "
            "Install the Qt 6 Gui and Network development packages to enable it, or pass "
            "-DQMLTUI_ENABLE_IMAGE=OFF to suppress this warning.")
        set(QMLTUI_ENABLE_IMAGE OFF)
        if(NOT PROJECT_IS_TOP_LEVEL)
            set(QMLTUI_ENABLE_IMAGE OFF PARENT_SCOPE)
        endif()
    endif()
endif()

qt_standard_project_setup(REQUIRES 6.8)

set(_qmltui_rpath_origin "")
if(APPLE)
    set(_qmltui_rpath_origin "@loader_path")
elseif(UNIX AND NOT EMSCRIPTEN)
    set(_qmltui_rpath_origin "$ORIGIN")
endif()

# Configures INSTALL_RPATH on <target> (installed under <install_dir>) so installed
# executables, libraries, and QML plugins resolve libQmlTui* in CMAKE_INSTALL_LIBDIR
# via $ORIGIN on ELF platforms and @loader_path on macOS.
function(qmltui_set_install_rpath target install_dir)
    if(NOT _qmltui_rpath_origin)
        return()
    endif()
    if(IS_ABSOLUTE "${install_dir}")
        set(_full_install_dir "${install_dir}")
    else()
        set(_full_install_dir "${CMAKE_INSTALL_PREFIX}/${install_dir}")
    endif()
    set(_full_libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
    cmake_path(NORMAL_PATH _full_install_dir)
    cmake_path(NORMAL_PATH _full_libdir)
    file(RELATIVE_PATH _rel_libdir "${_full_install_dir}" "${_full_libdir}")
    if(_rel_libdir STREQUAL "" OR _rel_libdir STREQUAL ".")
        set(_rpath "${_qmltui_rpath_origin}")
    else()
        string(REGEX REPLACE "/+$" "" _rel_libdir "${_rel_libdir}")
        set(_rpath "${_qmltui_rpath_origin};${_qmltui_rpath_origin}/${_rel_libdir}")
    endif()
    set_target_properties(${target} PROPERTIES INSTALL_RPATH "${_rpath}")
endfunction()

if(EMSCRIPTEN)
    include(cmake/QmlTuiWasm.cmake)
endif()

if(QMLTUI_BUILD_TESTS)
    include(CTest)
    enable_testing()
else()
    set(BUILD_TESTING OFF)
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

if(QMLTUI_BUILD_TESTS OR QMLTUI_BUILD_BENCHMARKS)
    add_subdirectory(tests)
endif()

configure_package_config_file(
    cmake/QmlTuiConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/QmlTuiConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QmlTui"
    PATH_VARS QMLTUI_INSTALL_QMLDIR
)
if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(_qmltui_version_compat SameMinorVersion)
else()
    set(_qmltui_version_compat SameMajorVersion)
endif()
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QmlTuiConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY ${_qmltui_version_compat}
)
unset(_qmltui_version_compat)
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
