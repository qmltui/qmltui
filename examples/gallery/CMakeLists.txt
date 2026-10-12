# QmlTui Gallery: one self-contained Main.qml, so the wasm playground can load it
# as a preset and the browser demo page can run it as an app.
set(gallery_qml_files Main.qml)
# The demos: complete standalone apps the launcher instantiates from their
# text (see Main.qml). Plain resources, not QML module files, so their names
# (Button.qml, ...) never shadow the QmlTui.Controls types they demonstrate.
file(GLOB gallery_example_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.qml")
# Behavior needs QMLTUI_ENABLE_PRIVATE_QT; without it the tile is left out and the
# catalog hides the entry (Main.qml filters on the files it was given).
if(NOT QMLTUI_ENABLE_PRIVATE_QT)
    list(FILTER gallery_example_files EXCLUDE REGEX "/Behavior\\.qml$")
endif()
if(NOT QMLTUI_ENABLE_IMAGE)
    list(FILTER gallery_example_files EXCLUDE REGEX "/(Image|AnimatedSprite)\\.qml$")
endif()
# The Image tile's picture and the AnimatedSprite tile's sheet, shared with
# examples/components/image and examples/components/animated_sprite.
set(gallery_asset_dir "${CMAKE_CURRENT_SOURCE_DIR}/../components/image")
set(gallery_sprite_dir "${CMAKE_CURRENT_SOURCE_DIR}/../components/animated_sprite")

function(gallery_add_executable target uri main_source)
    qt_add_executable(${target} ${main_source})
    set(gallery_dependencies TARGET qmltui::core TARGET qmltui::controls)
    set(gallery_link_libraries Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
    if(QMLTUI_ENABLE_IMAGE)
        list(APPEND gallery_dependencies TARGET qmltui::image)
        list(APPEND gallery_link_libraries qmltui::image)
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
    if(QMLTUI_ENABLE_IMAGE)
        qt_add_resources(${target} "${target}_assets"
            PREFIX "/gallery/assets"
            BASE "${gallery_asset_dir}"
            FILES "${gallery_asset_dir}/panda.png"
        )
        qt_add_resources(${target} "${target}_sprite_assets"
            PREFIX "/gallery/assets"
            BASE "${gallery_sprite_dir}"
            FILES "${gallery_sprite_dir}/loading.png"
        )
    endif()
    target_link_libraries(${target} PRIVATE ${gallery_link_libraries})
    foreach(plugin_target IN ITEMS
            QmlTuiCoreplugin QmlTuiControlsplugin QmlTuiImageplugin
            qmltui::corePlugin qmltui::controlsPlugin qmltui::imagePlugin)
        if(TARGET ${plugin_target})
            add_dependencies(${target} ${plugin_target})
        endif()
    endforeach()
endfunction()

gallery_add_executable(qmltui_gallery Gallery main.cpp)

if(COMMAND qmltui_wasm_app)   # browser build, see examples/wasm and docs/topics/webassembly.md
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
        LABELS AgentSmoke
    )

    qmltui_add_scenario(NAME gallery_idle_budget TARGET qmltui_gallery
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_idle_budget.js"
        COLUMNS 100 ROWS 30 LABELS AgentSmoke)
    qmltui_add_scenario(NAME gallery_tour TARGET qmltui_gallery
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_gallery_tour.js"
        COLUMNS 100 ROWS 30 CLOCK VIRTUAL RECORD gallery-tour.cast LABELS Cast)
endif()
