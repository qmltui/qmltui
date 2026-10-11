# The live QML editor's runtime. Qtui.Image pulls in Qt Gui (image decoders,
# fonts) and doubles the download; it is off unless asked for.
option(QTUI_WASM_PLAYGROUND_IMAGE "Link Qtui.Image into the playground binary" OFF)

set(playground_imports "import QtQml\nimport QtQml.Models\nimport Qtui\nimport Qtui.Controls\n")
set(playground_modules Qtui::Core Qtui::Controls)
if(QTUI_WASM_PLAYGROUND_IMAGE AND TARGET Qtui::Image)
    string(APPEND playground_imports "import Qtui.Image\n")
    list(APPEND playground_modules Qtui::Image)
endif()
# Never instantiated: its imports make qmlimportscanner link every QML plugin
# the playground offers into the static binary.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Imports.qml"
    "// Generated; see CMakeLists.txt.\n${playground_imports}\nQtObject {}\n")

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/Imports.qml"
    PROPERTIES QT_RESOURCE_ALIAS Imports.qml)

qt_add_executable(qtui_playground main.cpp)

qt_add_qml_module(qtui_playground
    URI QtuiPlayground
    VERSION 1.0
    QML_FILES "${CMAKE_CURRENT_BINARY_DIR}/Imports.qml"
    DEPENDENCIES TARGET ${playground_modules}
)

target_link_libraries(qtui_playground PRIVATE Qt6::Core Qt6::Qml ${playground_modules})

if(QTUI_WASM_PLAYGROUND_IMAGE)
    # The gallery's Image tile loads qrc:/gallery/assets/panda.png and its
    # AnimatedSprite tile qrc:/gallery/assets/qami.png, so the runtime carries
    # the same resources the desktop gallery embeds.
    set(gallery_asset_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../components/image")
    set(gallery_sprite_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../components/animated_sprite")
    qt_add_resources(qtui_playground playground_gallery_assets
        PREFIX "/gallery/assets"
        BASE "${gallery_asset_dir}"
        FILES "${gallery_asset_dir}/panda.png"
    )
    qt_add_resources(qtui_playground playground_gallery_sprite_assets
        PREFIX "/gallery/assets"
        BASE "${gallery_sprite_dir}"
        FILES "${gallery_sprite_dir}/qami.png"
    )
endif()

if(COMMAND qtui_wasm_app)
    qtui_wasm_app(qtui_playground)
endif()
