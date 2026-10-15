# QmlTui in the browser: the component gallery as a live playground. Builds the
# library and one generic runtime (playground/), copies the gallery's Main.qml
# next to it, and lays out the editor page. The page runs the gallery; opening
# a tile puts that tile's QML in the editor for live editing.
#
#   source ~/emsdk/emsdk_env.sh
#   <Qt>/wasm_singlethread/bin/qt-cmake -S examples/wasm -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
#   cmake --build build-wasm
#   cmake --build build-wasm --target serve        # http://localhost:8000/
#
# See docs/topics/webassembly.md. WebAssembly builds are GPLv3 or commercial only.
cmake_minimum_required(VERSION 3.21)
project(qmltuiWasmPlayground LANGUAGES CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Configure this project with the Qt for WebAssembly kit's qt-cmake; "
                        "see docs/topics/webassembly.md")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Only what the browser can run. Runner owns a std::thread and the kit is
# single-threaded; TerminalHost is a Unix PTY. Behavior is on: the wasm kit
# ships Qt6QmlPrivate, and the gallery has a Behavior tile. QmlTui.Image is on
# too for the Image tile; it adds Qt Gui to the download (see playground/).
set(QMLTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QMLTUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QMLTUI_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(QMLTUI_ENABLE_RUNNER OFF CACHE BOOL "" FORCE)
set(QMLTUI_ENABLE_TERMINAL_HOST OFF CACHE BOOL "" FORCE)
option(QMLTUI_ENABLE_PRIVATE_QT "Private Qt features: Behavior, StateGroup (needs Qt QmlPrivate)" ON)
option(QMLTUI_WASM_PLAYGROUND_IMAGE "Link QmlTui.Image into the playground binary" ON)

add_subdirectory(../.. qmltui)   # the root includes cmake/QmlTuiWasm.cmake under EMSCRIPTEN
add_subdirectory(playground)

qmltui_wasm_add_demo_page("${CMAKE_CURRENT_SOURCE_DIR}/www")

# The gallery is the app the page runs. configure_file() copies it and makes
# the build re-run configure when it changes.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../gallery/Main.qml" "${QMLTUI_WASM_WWW_DIR}/gallery.qml" COPYONLY)
# The demos it instantiates, plus an index the page fetches them from.
file(GLOB gallery_examples CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../gallery/examples/*.qml")
# Tiles the runtime cannot instantiate are left out; the gallery then hides them.
if(NOT QMLTUI_ENABLE_PRIVATE_QT)
    list(FILTER gallery_examples EXCLUDE REGEX "/Behavior\\.qml$")
endif()
if(NOT QMLTUI_WASM_PLAYGROUND_IMAGE OR NOT QMLTUI_ENABLE_IMAGE)
    list(FILTER gallery_examples EXCLUDE REGEX "/(Image|AnimatedSprite)\\.qml$")
endif()
set(example_index "")
foreach(example IN LISTS gallery_examples)
    get_filename_component(name "${example}" NAME)
    configure_file("${example}" "${QMLTUI_WASM_WWW_DIR}/examples/${name}" COPYONLY)
    string(APPEND example_index "\"${name}\", ")
endforeach()
string(REGEX REPLACE ", $" "" example_index "${example_index}")
file(WRITE "${QMLTUI_WASM_WWW_DIR}/examples/index.json" "[${example_index}]\n")
