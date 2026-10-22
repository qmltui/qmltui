cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# When built in-tree (`cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON`),
# `qmltui::core` is already provided by the root project.
# When built as a standalone project (`cmake -S examples/helloworld -B build`),
# acquire QmlTui using either Option 1 (`FetchContent`) or Option 2 (`find_package`).
if(NOT TARGET qmltui::core)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
    qt_standard_project_setup(REQUIRES 6.8)

    # --------------------------------------------------------------------------
    # Option 1: FetchContent (downloads and builds QmlTui from source)
    # --------------------------------------------------------------------------
    set(QMLTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(QMLTUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        QmlTui
        GIT_REPOSITORY https://github.com/qmltui-dev/qmltui.git
        GIT_TAG        v0.5.0
    )
    FetchContent_MakeAvailable(QmlTui)

    # --------------------------------------------------------------------------
    # Option 2: Installed package (`find_package`)
    # --------------------------------------------------------------------------
    # If QmlTui is installed (`cmake --install build --prefix /your/prefix`),
    # replace the FetchContent block above with:
    #
    #   find_package(QmlTui 0.5 REQUIRED)
    #   list(APPEND QT_QML_IMPORT_PATH "${QmlTui_QML_IMPORT_PATH}")
    #
    # Note: Qt's `qt_add_qml_module(DEPENDENCIES TARGET ...)` only accepts
    # non-imported build-tree targets. When using `find_package(QmlTui)`, omit
    # `DEPENDENCIES TARGET qmltui::core` from `qt_add_qml_module(...)` below.
endif()

qt_add_executable(hello_qmltui main.cpp)
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core
)
target_link_libraries(hello_qmltui PRIVATE Qt6::Core Qt6::Qml qmltui::core)

if(TARGET qmltui::runner)
    target_link_libraries(hello_qmltui PRIVATE qmltui::runner)
endif()
