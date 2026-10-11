# The live QML editor's runtime. Qamui.Image pulls in Qt Gui (image decoders,
# fonts) and doubles the download; it is off unless asked for.
option(QAMUI_WASM_PLAYGROUND_IMAGE "Link Qamui.Image into the playground binary" OFF)

set(playground_imports "import QtQml\nimport QtQml.Models\nimport Qamui\nimport Qamui.Controls\n")
set(playground_modules Qamui::Core Qamui::Controls)
if(QAMUI_WASM_PLAYGROUND_IMAGE AND TARGET Qamui::Image)
    string(APPEND playground_imports "import Qamui.Image\n")
    list(APPEND playground_modules Qamui::Image)
endif()
# Never instantiated: its imports make qmlimportscanner link every QML plugin
# the playground offers into the static binary.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Imports.qml"
    "// Generated; see CMakeLists.txt.\n${playground_imports}\nQtObject {}\n")

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/Imports.qml"
    PROPERTIES QT_RESOURCE_ALIAS Imports.qml)

qt_add_executable(qamui_playground main.cpp)

qt_add_qml_module(qamui_playground
    URI QamuiPlayground
    VERSION 1.0
    QML_FILES "${CMAKE_CURRENT_BINARY_DIR}/Imports.qml"
    DEPENDENCIES TARGET ${playground_modules}
)

target_link_libraries(qamui_playground PRIVATE Qt6::Core Qt6::Qml ${playground_modules})

if(QAMUI_WASM_PLAYGROUND_IMAGE)
    # The gallery's Image tile loads qrc:/gallery/assets/panda.png and its
    # AnimatedSprite tile qrc:/gallery/assets/loading.png, so the runtime carries
    # the same resources the desktop gallery embeds.
    set(gallery_asset_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../components/image")
    set(gallery_sprite_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../components/animated_sprite")
    qt_add_resources(qamui_playground playground_gallery_assets
        PREFIX "/gallery/assets"
        BASE "${gallery_asset_dir}"
        FILES "${gallery_asset_dir}/panda.png"
    )
    qt_add_resources(qamui_playground playground_gallery_sprite_assets
        PREFIX "/gallery/assets"
        BASE "${gallery_sprite_dir}"
        FILES "${gallery_sprite_dir}/loading.png"
    )
endif()

if(COMMAND qamui_wasm_app)
    qamui_wasm_app(qamui_playground)
endif()
