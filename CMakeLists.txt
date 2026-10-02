cmake_minimum_required(VERSION 3.20)
project(TerminalUI VERSION 0.1 LANGUAGES CXX)

option(QTUI_BUILD_EXAMPLES   "Build the examples" ON)
option(QTUI_BUILD_TESTS      "Build tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(QT_NO_PRIVATE_MODULE_WARNING ON CACHE BOOL "Suppress warnings about using private Qt modules")
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml QmlPrivate)
if(QTUI_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

qt_standard_project_setup(REQUIRES 6.8)

if(QTUI_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

add_subdirectory(Terminal)
add_subdirectory(qtasktree_app)

# Example executable to test the library
if(QTUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(QTUI_BUILD_TESTS)
    add_subdirectory(tests)
endif()
