cmake_minimum_required(VERSION 3.20)
project(Qtui VERSION 0.1 LANGUAGES CXX)

option(QTUI_BUILD_EXAMPLES   "Build the examples" ${PROJECT_IS_TOP_LEVEL})
option(QTUI_BUILD_TESTS      "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(QTUI_BUILD_BENCHMARKS "Build performance benchmark executables" OFF)
option(QTUI_ENABLE_BEHAVIOR  "Enable Behavior on properties (requires Qt QmlPrivate)" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(QTUI_INSTALL_QMLDIR "${CMAKE_INSTALL_LIBDIR}/qt6/qml" CACHE PATH
    "Install location for Qtui QML modules")

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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
if(QTUI_ENABLE_BEHAVIOR)
    set(QT_NO_PRIVATE_MODULE_WARNING ON CACHE BOOL
        "Suppress warnings about the optional Qt QmlPrivate dependency")
    find_package(Qt6 6.8 REQUIRED COMPONENTS QmlPrivate)
endif()
if(QTUI_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

qt_standard_project_setup(REQUIRES 6.8)
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

if(QTUI_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

add_subdirectory(Terminal)

# Example executable to test the library
if(QTUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
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
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qtui"
)
