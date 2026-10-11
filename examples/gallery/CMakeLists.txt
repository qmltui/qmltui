# Qamui Gallery: one self-contained Main.qml, so the wasm playground can load it
# as a preset and the browser demo page can run it as an app.
set(gallery_qml_files Main.qml)
# The demos: complete standalone apps the launcher instantiates from their
# text (see Main.qml). Plain resources, not QML module files, so their names
# (Button.qml, ...) never shadow the Qamui.Controls types they demonstrate.
file(GLOB gallery_example_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.qml")
# Behavior needs QAMUI_ENABLE_BEHAVIOR; without it the tile is left out and the
# catalog hides the entry (Main.qml filters on the files it was given).
if(NOT QAMUI_ENABLE_BEHAVIOR)
    list(FILTER gallery_example_files EXCLUDE REGEX "/Behavior\\.qml$")
endif()
if(NOT QAMUI_ENABLE_IMAGE)
    list(FILTER gallery_example_files EXCLUDE REGEX "/(Image|AnimatedSprite)\\.qml$")
endif()
# The Image tile's picture and the AnimatedSprite tile's sheet, shared with
# examples/components/image and examples/components/animated_sprite.
set(gallery_asset_dir "${CMAKE_CURRENT_SOURCE_DIR}/../components/image")
set(gallery_sprite_dir "${CMAKE_CURRENT_SOURCE_DIR}/../components/animated_sprite")

function(gallery_add_executable target uri main_source)
    qt_add_executable(${target} ${main_source})
    set(gallery_dependencies TARGET Qamui::Core TARGET Qamui::Controls)
    set(gallery_link_libraries Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
    if(QAMUI_ENABLE_IMAGE)
        list(APPEND gallery_dependencies TARGET Qamui::Image)
        list(APPEND gallery_link_libraries Qamui::Image)
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
    if(QAMUI_ENABLE_IMAGE)
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
            QamuiCoreplugin QamuiControlsplugin QamuiImageplugin
            Qamui::CorePlugin Qamui::ControlsPlugin Qamui::ImagePlugin)
        if(TARGET ${plugin_target})
            add_dependencies(${target} ${plugin_target})
        endif()
    endforeach()
endfunction()

gallery_add_executable(qamui_gallery Gallery main.cpp)

if(COMMAND qamui_wasm_app)   # browser build, see examples/wasm and docs/topics/webassembly.md
    qamui_wasm_app(qamui_gallery)
endif()

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_gallery PRIVATE Qamui::Runner)

    # Gallery pages animate on QML Timers and Qamui animations: the tour
    # runs on the virtual clock, so its per-page settling is instant.
    qamui_add_scenario(
        NAME gallery_smoke
        TARGET qamui_gallery
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_gallery.js"
        COLUMNS 100
        ROWS 30
        CLOCK VIRTUAL
        ARTIFACT "gallery/grid.txt"
        ARTIFACT_CONTENT "Qamui Gallery"
        LABELS AgentSmoke
    )

    qamui_add_scenario(NAME gallery_idle_budget TARGET qamui_gallery
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_idle_budget.js"
        COLUMNS 100 ROWS 30 LABELS AgentSmoke)
    qamui_add_scenario(NAME gallery_tour TARGET qamui_gallery
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_gallery_tour.js"
        COLUMNS 100 ROWS 30 CLOCK VIRTUAL RECORD gallery-tour.cast LABELS Cast)
endif()
