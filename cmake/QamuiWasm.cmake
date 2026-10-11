# WebAssembly support, included by the root CMakeLists.txt under if(EMSCRIPTEN).
#
# Qt's own executable finalizer already links every wasm app with MODULARIZE,
# EXPORT_NAME (<target>_entry), embind, the exported runtime methods, and the
# memory settings. This file adds only what a Qamui terminal app needs on top:
# no Qt shell page (there is no canvas to draw on), a common output directory
# the demo page can be served from, and the demo page itself.

set(QAMUI_WASM_WWW_DIR "${CMAKE_BINARY_DIR}/www" CACHE PATH
    "Directory that holds the .js/.wasm artifacts and the demo page")
set(QAMUI_WASM_INITIAL_MEMORY "" CACHE STRING
    "Override Qt's default -sINITIAL_MEMORY (50MB) for Qamui apps, e.g. 96MB")

# Marks an executable as a Qamui terminal app for the browser (the playground
# runtime, or any app built standalone). Call it in the directory that created
# the target: Qt's deferred finalizer reads these properties at the end of that
# directory scope.
function(qamui_wasm_app target)
    set_target_properties(${target} PROPERTIES
        NO_WASM_DEFAULT_FILES ON
        RUNTIME_OUTPUT_DIRECTORY "${QAMUI_WASM_WWW_DIR}"
    )
    if(QAMUI_WASM_INITIAL_MEMORY)
        set_target_properties(${target} PROPERTIES
            QT_WASM_INITIAL_MEMORY "${QAMUI_WASM_INITIAL_MEMORY}")
    endif()
endfunction()

# Copies the editor page next to the artifacts and adds a `serve` target.
# `source_dir` holds index.html and the page scripts, copied as they are: no
# templating, because configure_file's @VAR@ substitution would eat the
# @scope/package@version CDN URLs in them.
function(qamui_wasm_add_demo_page source_dir)
    foreach(page index.html qamui-playground.js qamui-completion.js)
        configure_file("${source_dir}/${page}" "${QAMUI_WASM_WWW_DIR}/${page}" COPYONLY)
    endforeach()

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_custom_target(serve
            COMMAND "${Python3_EXECUTABLE}" -m http.server 8000 --directory "${QAMUI_WASM_WWW_DIR}"
            USES_TERMINAL VERBATIM
            COMMENT "Serving ${QAMUI_WASM_WWW_DIR} at http://localhost:8000/"
        )
    endif()
endfunction()
