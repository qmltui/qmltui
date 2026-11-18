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
    include(CTest)

    # --------------------------------------------------------------------------
    # Option 1: FetchContent (downloads and builds QmlTui from source)
    # --------------------------------------------------------------------------
    include(FetchContent)
    FetchContent_Declare(
        QmlTui
        GIT_REPOSITORY https://github.com/qmltui/qmltui.git
        GIT_TAG        main # Pin a release tag (e.g. v0.6.0) once published
    )
    FetchContent_MakeAvailable(QmlTui)

    # --------------------------------------------------------------------------
    # Option 2: Installed package (`find_package`)
    # --------------------------------------------------------------------------
    # If QmlTui is installed (`cmake --install build --prefix /your/prefix`),
    # replace the FetchContent block above with:
    #
    #   find_package(QmlTui 0.6 REQUIRED)
endif()

set(QT_QML_GENERATE_QMLLS_INI ON)

# Order QmlTui's build-tree QML metadata before qmlcachegen/qmllint when built
# from source; find_package(QmlTui) provides IMPORTED targets and configures
# QT_QML_IMPORT_PATH automatically instead.
set(_hello_qml_dependencies)
get_target_property(_qmltui_imported qmltui::core IMPORTED)
if(NOT _qmltui_imported)
    list(APPEND _hello_qml_dependencies DEPENDENCIES TARGET qmltui::core)
endif()
unset(_qmltui_imported)

qt_add_executable(hello_qmltui main.cpp)
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    QML_FILES Main.qml
    ${_hello_qml_dependencies}
)
unset(_hello_qml_dependencies)
target_link_libraries(hello_qmltui PRIVATE Qt6::Core Qt6::Qml qmltui::core)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(hello_qmltui)
endif()

if(BUILD_TESTING AND TARGET qmltui::runner)
    target_link_libraries(hello_qmltui PRIVATE qmltui::runner)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_helloworld.js")
        enable_testing()
        qmltui_add_scenario(
            NAME hello_world
            TARGET hello_qmltui
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_helloworld.js"
            COLUMNS 64
            ROWS 15
            CLOCK VIRTUAL
        )
        qmltui_add_scenario(
            NAME hello_world_demo
            TARGET hello_qmltui
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_helloworld.js"
            COLUMNS 64
            ROWS 15
            CLOCK REAL
            RECORD hello-world.cast
        )
    endif()
endif()
