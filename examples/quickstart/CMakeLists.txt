cmake_minimum_required(VERSION 3.21)
project(hello_qtui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Fetch Qtui at a release tag: nothing to install besides Qt.
set(QTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QTUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
    Qtui
    GIT_REPOSITORY https://github.com/yosriayed/Qtui.git
    GIT_TAG        v0.4.0
)
FetchContent_MakeAvailable(Qtui)

qt_add_executable(hello_qtui main.cpp)
qt_add_qml_module(hello_qtui
    URI HelloQtui
    VERSION 1.0
    QML_FILES Main.qml
 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)
target_link_libraries(hello_qtui PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
