# QmlTui Gallery: one self-contained Main.qml, so the wasm playground can load it
# as a preset and the browser demo page can run it as an app.
set(gallery_qml_files Main.qml)
# The component demos: Item-rooted QML files loaded by Main.qml's Loader or run
# standalone via `qmltui_gallery --example <Name>`. Mounted as plain resources
# under /qt/qml/Gallery/examples (not QML_FILES) so their names (Button.qml, ...)
# never register as QML types that would shadow QmlTui.Controls.
file(GLOB gallery_example_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.qml")
# Behavior needs QMLTUI_ENABLE_PRIVATE_QT; without it the tile is left out and the
# catalog hides the entry (Main.qml filters on the files it was given).
if(NOT QMLTUI_ENABLE_PRIVATE_QT)
    list(FILTER gallery_example_files EXCLUDE REGEX "/Behavior\\.qml$")
endif()
if(NOT QMLTUI_ENABLE_IMAGE)
    list(FILTER gallery_example_files EXCLUDE REGEX "/(Image|AnimatedSprite)\\.qml$")
endif()
if(NOT QMLTUI_ENABLE_PTYVIEW)
    list(FILTER gallery_example_files EXCLUDE REGEX "/PtyView\\.qml$")
endif()
set(gallery_asset_dir "${CMAKE_CURRENT_SOURCE_DIR}/assets")

function(gallery_add_executable target uri main_source)
    qt_add_executable(${target} ${main_source})
    set(gallery_dependencies TARGET qmltui::core TARGET qmltui::controls)
    set(gallery_link_libraries Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
    if(QMLTUI_ENABLE_IMAGE)
        list(APPEND gallery_dependencies TARGET qmltui::image)
        list(APPEND gallery_link_libraries qmltui::image)
    endif()
    if(QMLTUI_ENABLE_PTYVIEW)
        list(APPEND gallery_dependencies TARGET qmltui::pty_view)
        list(APPEND gallery_link_libraries qmltui::pty_view)
    endif()
    qt_add_qml_module(${target}
        URI ${uri}
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml"
        QML_FILES ${gallery_qml_files}
        DEPENDENCIES ${gallery_dependencies}
    )
    qt_add_resources(${target} "${target}_examples"
        PREFIX "/qt/qml/Gallery/examples"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/examples"
        FILES ${gallery_example_files}
    )
    if(QMLTUI_ENABLE_IMAGE)
        qt_add_resources(${target} "${target}_assets"
            PREFIX "/gallery/assets"
            BASE "${gallery_asset_dir}"
            FILES
                "${gallery_asset_dir}/panda.png"
                "${gallery_asset_dir}/loading.png"
        )
    endif()
    target_link_libraries(${target} PRIVATE ${gallery_link_libraries})
    foreach(plugin_target IN ITEMS
            QmlTuiCoreplugin QmlTuiControlsplugin QmlTuiImageplugin QmlTuiPtyViewplugin
            qmltui::corePlugin qmltui::controlsPlugin qmltui::imagePlugin qmltui::pty_viewPlugin)
        if(TARGET ${plugin_target})
            add_dependencies(${target} ${plugin_target})
        endif()
    endforeach()
endfunction()

gallery_add_executable(qmltui_gallery Gallery main.cpp)

if(COMMAND qmltui_wasm_app)   # browser build, see examples/wasm and docs/guides/webassembly.md
    qmltui_wasm_app(qmltui_gallery)
endif()

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_gallery PRIVATE qmltui::runner)

    # Gallery pages animate on QML Timers and QmlTui animations: the tour
    # runs on the virtual clock, so its per-page settling is instant.
    qmltui_add_scenario(
        NAME gallery_smoke
        TARGET qmltui_gallery
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_gallery.js"
        COLUMNS 100
        ROWS 30
        CLOCK VIRTUAL
        ARTIFACT "gallery/grid.txt"
        ARTIFACT_CONTENT "QmlTui Gallery"
    )

    qmltui_add_scenario(NAME gallery_idle_budget TARGET qmltui_gallery
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_idle_budget.js"
        COLUMNS 100 ROWS 30)
    qmltui_add_scenario(NAME gallery_tour TARGET qmltui_gallery
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_gallery_tour.js"
        COLUMNS 100 ROWS 30 CLOCK REAL SCENARIO_TIMEOUT_MS 90000
        RECORD gallery-tour.cast)

    # Standalone .cast + .gif recording scenarios for each built-in type in examples/gallery/examples/*.qml
    foreach(example_file IN LISTS gallery_example_files)
        get_filename_component(example_stem "${example_file}" NAME_WE)
        string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" example_snake "${example_stem}")
        string(TOLOWER "${example_snake}" example_snake)
        set(scenario_script "${CMAKE_CURRENT_SOURCE_DIR}/scenarios/${example_snake}.js")
        if(EXISTS "${scenario_script}")
            qmltui_add_scenario(
                NAME "gallery_${example_snake}_demo"
                TARGET qmltui_gallery
                ARGUMENTS --example "${example_stem}"
                SCRIPT "${scenario_script}"
                COLUMNS 84
                ROWS 20
                CLOCK REAL
                RECORD "components/${example_snake}.cast"
            )
        endif()
    endforeach()
endif()
