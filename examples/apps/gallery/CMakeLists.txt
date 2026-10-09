# Qtui Gallery: one self-contained Main.qml, so the wasm playground can load it
# as a preset and the browser demo page can run it as an app.
set(gallery_qml_files Main.qml)
# The demos: complete standalone apps the launcher instantiates from their
# text (see Main.qml). Plain resources, not QML module files, so their names
# (Button.qml, ...) never shadow the Qtui.Controls types they demonstrate.
file(GLOB gallery_example_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.qml")
# Behavior needs QTUI_ENABLE_BEHAVIOR; without it the tile is left out and the
# catalog hides the entry (Main.qml filters on the files it was given).
if(NOT QTUI_ENABLE_BEHAVIOR)
    list(FILTER gallery_example_files EXCLUDE REGEX "/Behavior\\.qml$")
endif()
if(NOT QTUI_ENABLE_IMAGE)
    list(FILTER gallery_example_files EXCLUDE REGEX "/Image\\.qml$")
endif()
# The Image tile's picture, shared with examples/components/image.
set(gallery_asset_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../components/image")

function(gallery_add_executable target uri main_source)
    qt_add_executable(${target} ${main_source})
    set(gallery_dependencies TARGET Qtui::Core TARGET Qtui::Controls)
    set(gallery_link_libraries Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
    if(QTUI_ENABLE_IMAGE)
        list(APPEND gallery_dependencies TARGET Qtui::Image)
        list(APPEND gallery_link_libraries Qtui::Image)
    endif()
    qt_add_qml_module(${target}
        URI ${uri}
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml"
        QML_FILES ${gallery_qml_files}
        DEPENDENCIES ${gallery_dependencies}
    )
    qt_add_resources(${target} "${target}_examples"
        PREFIX "/gallery/examples"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/examples"
        FILES ${gallery_example_files}
    )
    if(QTUI_ENABLE_IMAGE)
        qt_add_resources(${target} "${target}_assets"
            PREFIX "/gallery/assets"
            BASE "${gallery_asset_dir}"
            FILES "${gallery_asset_dir}/panda.png"
        )
    endif()
    target_link_libraries(${target} PRIVATE ${gallery_link_libraries})
    foreach(plugin_target IN ITEMS
            QtuiCoreplugin QtuiControlsplugin QtuiImageplugin
            Qtui::CorePlugin Qtui::ControlsPlugin Qtui::ImagePlugin)
        if(TARGET ${plugin_target})
            add_dependencies(${target} ${plugin_target})
        endif()
    endforeach()
endfunction()

gallery_add_executable(qtui_gallery Gallery main.cpp)

if(COMMAND qtui_wasm_app)   # browser build, see examples/wasm and docs/guide/webassembly.md
    qtui_wasm_app(qtui_gallery)
endif()

if(TARGET Qtui::Runner)
    gallery_add_executable(qtui_gallery_test GalleryTest main_test.cpp)
    target_link_libraries(qtui_gallery_test PRIVATE Qtui::Runner)

    if(QTUI_BUILD_TESTS)
        include("${PROJECT_SOURCE_DIR}/cmake/QtuiRunner.cmake")
        qtui_add_scenario_test(
            NAME gallery_smoke
            TARGET qtui_gallery_test
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_gallery.js"
            COLUMNS 100
            ROWS 30
            ARTIFACT "gallery/grid.txt"
            ARTIFACT_CONTENT "Qtui Gallery"
            LABELS AgentSmoke
        )
    endif()
endif()
