# Qtui in the browser: the component gallery as a live playground. Builds the
# library and one generic runtime (playground/), copies the gallery's Main.qml
# next to it, and lays out the editor page. The page runs the gallery; opening
# a tile puts that tile's QML in the editor for live editing.
#
#   source ~/emsdk/emsdk_env.sh
#   <Qt>/wasm_singlethread/bin/qt-cmake -S examples/wasm -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
#   cmake --build build-wasm
#   cmake --build build-wasm --target serve        # http://localhost:8000/
#
# See docs/guide/webassembly.md. WebAssembly builds are GPLv3 or commercial only.
cmake_minimum_required(VERSION 3.21)
project(QtuiWasmPlayground LANGUAGES CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Configure this project with the Qt for WebAssembly kit's qt-cmake; "
                        "see docs/guide/webassembly.md")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Only what the browser can run. TestRunner owns a std::thread and the kit is
# single-threaded; TerminalHost is a Unix PTY. Behavior is on: the wasm kit
# ships Qt6QmlPrivate, and the gallery has a Behavior tile. Qtui.Image is on
# too for the Image tile; it adds Qt Gui to the download (see playground/).
set(QTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QTUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QTUI_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(QTUI_ENABLE_TEST_RUNNER OFF CACHE BOOL "" FORCE)
set(QTUI_ENABLE_TERMINAL_HOST OFF CACHE BOOL "" FORCE)
option(QTUI_ENABLE_BEHAVIOR "Behavior on properties (needs Qt QmlPrivate)" ON)
option(QTUI_WASM_PLAYGROUND_IMAGE "Link Qtui.Image into the playground binary" ON)

add_subdirectory(../.. qtui)   # the root includes cmake/QtuiWasm.cmake under EMSCRIPTEN
add_subdirectory(playground)

qtui_wasm_add_demo_page("${CMAKE_CURRENT_SOURCE_DIR}/www")

# The gallery is the app the page runs. configure_file() copies it and makes
# the build re-run configure when it changes.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../apps/gallery/Main.qml" "${QTUI_WASM_WWW_DIR}/gallery.qml" COPYONLY)
# The demos it instantiates, plus an index the page fetches them from.
file(GLOB gallery_examples CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../apps/gallery/examples/*.qml")
# Tiles the runtime cannot instantiate are left out; the gallery then hides them.
if(NOT QTUI_ENABLE_BEHAVIOR)
    list(FILTER gallery_examples EXCLUDE REGEX "/Behavior\\.qml$")
endif()
if(NOT QTUI_WASM_PLAYGROUND_IMAGE)
    list(FILTER gallery_examples EXCLUDE REGEX "/Image\\.qml$")
endif()
set(example_index "")
foreach(example IN LISTS gallery_examples)
    get_filename_component(name "${example}" NAME)
    configure_file("${example}" "${QTUI_WASM_WWW_DIR}/examples/${name}" COPYONLY)
    string(APPEND example_index "\"${name}\", ")
endforeach()
string(REGEX REPLACE ", $" "" example_index "${example_index}")
file(WRITE "${QTUI_WASM_WWW_DIR}/examples/index.json" "[${example_index}]\n")

# Completion index for the editor, from the same qmltypes the docs use. The
# Qtui qmltypes appear at build time, so this is a build step, not configure.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    set(type_index_modules --module Qtui --module Qtui.Controls --module QML --module QtQml --module QtQml.Models)
    if(QTUI_WASM_PLAYGROUND_IMAGE)
        list(APPEND type_index_modules --module Qtui.Image)
    endif()
    add_custom_command(
        OUTPUT "${QTUI_WASM_WWW_DIR}/types.json"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/qml_type_index.py"
                --build-dir "${CMAKE_BINARY_DIR}" --qt-qml-dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}"
                ${type_index_modules} --output "${QTUI_WASM_WWW_DIR}/types.json"
        DEPENDS QtuiCore QtuiControls
                "${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/qml_type_index.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/component_docs.py"
        COMMENT "Generating the playground completion index"
        VERBATIM)
    add_custom_target(qtui_playground_types ALL DEPENDS "${QTUI_WASM_WWW_DIR}/types.json")
endif()
