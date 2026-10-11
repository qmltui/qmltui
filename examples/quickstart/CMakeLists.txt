cmake_minimum_required(VERSION 3.21)
project(hello_qamui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Fetch Qamui at a release tag: nothing to install besides Qt.
set(QAMUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QAMUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
    Qamui
    GIT_REPOSITORY https://github.com/qamui-dev/qamui.git
    GIT_TAG        v0.5.0
)
FetchContent_MakeAvailable(Qamui)

qt_add_executable(hello_qamui main.cpp)
qt_add_qml_module(hello_qamui
    URI HelloQamui
    VERSION 1.0
    QML_FILES Main.qml
 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)
target_link_libraries(hello_qamui PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
