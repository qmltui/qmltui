# Qamui in the browser: the component gallery as a live playground. Builds the
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
project(QamuiWasmPlayground LANGUAGES CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Configure this project with the Qt for WebAssembly kit's qt-cmake; "
                        "see docs/guide/webassembly.md")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Only what the browser can run. Runner owns a std::thread and the kit is
# single-threaded; TerminalHost is a Unix PTY. Behavior is on: the wasm kit
# ships Qt6QmlPrivate, and the gallery has a Behavior tile. Qamui.Image is on
# too for the Image tile; it adds Qt Gui to the download (see playground/).
set(QAMUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QAMUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QAMUI_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(QAMUI_ENABLE_TEST_RUNNER OFF CACHE BOOL "" FORCE)
set(QAMUI_ENABLE_TERMINAL_HOST OFF CACHE BOOL "" FORCE)
option(QAMUI_ENABLE_BEHAVIOR "Behavior on properties (needs Qt QmlPrivate)" ON)
option(QAMUI_WASM_PLAYGROUND_IMAGE "Link Qamui.Image into the playground binary" ON)

add_subdirectory(../.. qamui)   # the root includes cmake/QamuiWasm.cmake under EMSCRIPTEN
add_subdirectory(playground)

qamui_wasm_add_demo_page("${CMAKE_CURRENT_SOURCE_DIR}/www")

# The gallery is the app the page runs. configure_file() copies it and makes
# the build re-run configure when it changes.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../apps/gallery/Main.qml" "${QAMUI_WASM_WWW_DIR}/gallery.qml" COPYONLY)
# The demos it instantiates, plus an index the page fetches them from.
file(GLOB gallery_examples CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../apps/gallery/examples/*.qml")
# Tiles the runtime cannot instantiate are left out; the gallery then hides them.
if(NOT QAMUI_ENABLE_BEHAVIOR)
    list(FILTER gallery_examples EXCLUDE REGEX "/Behavior\\.qml$")
endif()
if(NOT QAMUI_WASM_PLAYGROUND_IMAGE OR NOT QAMUI_ENABLE_IMAGE)
    list(FILTER gallery_examples EXCLUDE REGEX "/(Image|AnimatedSprite)\\.qml$")
endif()
set(example_index "")
foreach(example IN LISTS gallery_examples)
    get_filename_component(name "${example}" NAME)
    configure_file("${example}" "${QAMUI_WASM_WWW_DIR}/examples/${name}" COPYONLY)
    string(APPEND example_index "\"${name}\", ")
endforeach()
string(REGEX REPLACE ", $" "" example_index "${example_index}")
file(WRITE "${QAMUI_WASM_WWW_DIR}/examples/index.json" "[${example_index}]\n")

# Completion index for the editor, from the same qmltypes the docs use. The
# Qamui qmltypes appear at build time, so this is a build step, not configure.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    set(type_index_modules --module Qamui --module Qamui.Controls --module QML --module QtQml --module QtQml.Models)
    if(QAMUI_WASM_PLAYGROUND_IMAGE)
        list(APPEND type_index_modules --module Qamui.Image)
    endif()
    add_custom_command(
        OUTPUT "${QAMUI_WASM_WWW_DIR}/types.json"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/qml_type_index.py"
                --build-dir "${CMAKE_BINARY_DIR}" --qt-qml-dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}"
                ${type_index_modules} --output "${QAMUI_WASM_WWW_DIR}/types.json"
        DEPENDS QamuiCore QamuiControls
                "${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/qml_type_index.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/component_docs.py"
        COMMENT "Generating the playground completion index"
        VERBATIM)
    add_custom_target(qamui_playground_types ALL DEPENDS "${QAMUI_WASM_WWW_DIR}/types.json")
endif()
