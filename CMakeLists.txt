cmake_minimum_required(VERSION 3.20)
project(Qtui VERSION 0.2.0 LANGUAGES CXX)

option(QTUI_BUILD_EXAMPLES   "Build the examples" ${PROJECT_IS_TOP_LEVEL})
option(QTUI_BUILD_TESTS      "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(QTUI_BUILD_BENCHMARKS "Build performance benchmark executables" OFF)
option(QTUI_ENABLE_BEHAVIOR  "Enable Behavior on properties (requires Qt QmlPrivate)" OFF)
option(QTUI_BUILD_TERMINAL_HOST "Build the optional Unix PTY TerminalView module" OFF)
option(QTUI_ENABLE_TEST_RUNNER "Build and install the optional Qtui Testing library" OFF)
option(QTUI_ENABLE_TEST_DRIVER "Alias for QTUI_ENABLE_TEST_RUNNER" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(QTUI_INSTALL_QMLDIR "${CMAKE_INSTALL_LIBDIR}/qt6/qml" CACHE PATH
    "Install location for Qtui QML modules")
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH
    "Build-tree location for generated QML modules")

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

if(QTUI_BUILD_TERMINAL_HOST)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
        message(FATAL_ERROR "QTUI_BUILD_TERMINAL_HOST currently supports only Linux and macOS")
    endif()

    enable_language(C)
    FetchContent_Declare(libvterm
        URL https://www.leonerd.org.uk/code/libvterm/libvterm-0.3.3.tar.gz
        URL_HASH SHA256=09156f43dd2128bd347cbeebe50d9a571d32c64e0cf18d211197946aff7226e0
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(libvterm)

    add_library(qtui_libvterm STATIC
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
    target_compile_features(qtui_libvterm PRIVATE c_std_99)
    target_include_directories(qtui_libvterm PUBLIC ${libvterm_SOURCE_DIR}/include)
    set_target_properties(qtui_libvterm PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
    )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml Gui)

if(QTUI_ENABLE_BEHAVIOR)
    set(QT_NO_PRIVATE_MODULE_WARNING ON CACHE BOOL
        "Suppress warnings about the optional Qt QmlPrivate dependency")
    find_package(Qt6 6.8 REQUIRED COMPONENTS QmlPrivate)
endif()
if(QTUI_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
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
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QtuiConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
install(EXPORT QtuiTargets
    FILE QtuiTargets.cmake
    NAMESPACE Qtui::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qtui"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QtuiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QtuiConfigVersion.cmake"
    cmake/QtuiTesting.cmake
    cmake/RunQtuiScenario.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qtui"
)
