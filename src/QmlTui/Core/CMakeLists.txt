include("${qmltui_SOURCE_DIR}/cmake/CheckEngineIncludes.cmake")

set(qmltui_private_qt_sources)
if(QMLTUI_ENABLE_PRIVATE_QT)
    list(APPEND qmltui_private_qt_sources
        Engine/Behavior.h
        Engine/Behavior.cpp
    )
endif()

# One terminal backend per platform behind Primitives/TerminalBackend.h.
set(qmltui_platform_sources)
if(WIN32)
    list(APPEND qmltui_platform_sources
        Primitives/Win32ConsoleBackend.h
        Primitives/Win32ConsoleBackend.cpp
    )
elseif(EMSCRIPTEN)
    list(APPEND qmltui_platform_sources
        Primitives/WasmTerminalBackend.h
        Primitives/WasmTerminalBackend.cpp
    )
else()
    list(APPEND qmltui_platform_sources
        Primitives/PosixTerminalBackend.h
        Primitives/PosixTerminalBackend.cpp
    )
endif()

qt_add_qml_module(QmlTuiCore
    URI QmlTui
    IMPORTS
        QtQml
    ${QMLTUI_STATIC_QML_MODULE}
    OUTPUT_TARGETS qmltui_core_output_targets
    SOURCES
        Primitives/Span.h
        Primitives/Span.cpp
        Primitives/TextStyle.h
        Primitives/TextStyle.cpp
        Primitives/HighlightRange.h
        Primitives/Color.h
        Primitives/Color.cpp
        Primitives/QmlTuiExport.h
        Primitives/QmlTuiGlobal.h
        Primitives/QmlTuiGlobal.cpp
        Primitives/InputDecoder.cpp
        Primitives/Cell.h
        Primitives/Cell.cpp
        Primitives/UnicodeText.h
        Primitives/UnicodeText.cpp
        Primitives/Subpixel.h
        Primitives/Subpixel.cpp
        Primitives/TextGridBuffer.h
        Primitives/TextGridBuffer.cpp
        Primitives/TextGridBufferAnsi.cpp
        Primitives/AnsiEncoder_p.h
        Primitives/TerminalCapabilities.h
        Primitives/TerminalCapabilities.cpp
        Primitives/TerminalBackend.h
        Primitives/TerminalBackend.cpp
        ${qmltui_platform_sources}
        Primitives/TerminalDevice.h
        Primitives/TerminalDevice.cpp
        Engine/Clipboard.h
        Engine/Clipboard.cpp
        Engine/Clock_p.h
        Engine/Clock.cpp
        Engine/TerminalEngine.h
        Engine/TerminalEngine.cpp
        Engine/TerminalMessageRouter_p.h
        Engine/TerminalMessageRouter.cpp
        Engine/QtMessageRouting.h
        Engine/QtMessageRouting.cpp
        Engine/InputRouter_p.h
        Engine/InputRouter.cpp
        Engine/InputScope.h
        Engine/FocusManager.h
        Engine/FocusManager.cpp
        Engine/Renderer.h
        Engine/RenderPhase_p.h
        Engine/RenderPhase.cpp
        Engine/ItemPaintTraversal_p.h
        Engine/AltScreenRenderer.h
        Engine/AltScreenRenderer.cpp
        Engine/InlineRenderer.h
        Engine/InlineRenderer.cpp
        Engine/Shortcut.h
        Engine/Shortcut.cpp
        Engine/KeyEvent.h
        Engine/MouseEvent.h
        Engine/Keys.h
        Engine/Keys.cpp
        Engine/Animation.h
        Engine/Animation.cpp
        Engine/State.h
        Engine/State.cpp
        Engine/StateGroup.h
        Engine/StateGroup.cpp
        Engine/Transition.h
        Engine/Transition.cpp
        Engine/QmlPrivateBridge_p.h
        Engine/QmlPrivateBridge.cpp
        Engine/StateController_p.h
        Engine/StateController.cpp
        ${qmltui_private_qt_sources}
        Items/Item.h
        Items/Item_p.h
        Items/Item.cpp
        Items/FocusScope.h
        Items/FocusScope.cpp
        Items/Canvas.h
        Items/Canvas.cpp
        Items/Rectangle.h
        Items/Rectangle.cpp
        Items/Anchors.h
        Items/Anchors.cpp
        Items/AnchorLine.h
        Items/Border.h
        Items/Border.cpp
        Items/TextInput.h
        Items/TextInput.cpp
        Items/TextEditorState.h
        Items/TextEditorState.cpp
        Items/TextArea.h
        Items/TextArea.cpp
        Items/TextCursorPainter.h
        Items/StackLayout.h
        Items/StackLayout.cpp
        Items/ScrollView.h
        Items/ScrollView.cpp
        Items/GridLayout.h
        Items/GridLayout.cpp
        Items/Terminal.h
        Items/Terminal.cpp
        Items/Popup.h
        Items/Popup.cpp
        Items/Text.h
        Items/Text.cpp
        Items/GradientStop.h
        Items/GradientStop.cpp
        Items/Gradient.h
        Items/Gradient.cpp
        Items/Scrollback.h
        Items/Scrollback.cpp
        Items/DelegateUtils.h
        Items/ModelSignalConnections_p.h
        Items/NumericItemRoles_p.h
        Items/Row.h
        Items/Row.cpp
        Items/Column.h
        Items/Column.cpp
        Items/Layout.h
        Items/Layout.cpp
        Items/LinearLayout.h
        Items/LinearLayout.cpp
        Items/MouseArea.h
        Items/MouseArea.cpp
        Items/Repeater.h
        Items/Repeater.cpp
        Items/Loader.h
        Items/Loader.cpp
        Items/ListView.h
        Items/ListView.cpp
        Items/GridView.h
        Items/GridView.cpp
        Items/TableView.h
        Items/TableView.cpp
        Items/TreeView.h
        Items/TreeView.cpp
)

# Headers are reached as <QmlTui/Core/<layer>/Name.h> from the src/ root in the
# build tree and from the include root once installed. The private entries
# serve the generated QML type registrations, which include each registered
# header by its bare file name and need its directory on the include path.
target_include_directories(QmlTuiCore
    PUBLIC
        $<BUILD_INTERFACE:${qmltui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Primitives
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine
        ${CMAKE_CURRENT_SOURCE_DIR}/Items
)

target_link_libraries(QmlTuiCore PUBLIC
    Qt6::Qml
)
if(EMSCRIPTEN)
    target_link_libraries(QmlTuiCore PUBLIC embind)
endif()

target_link_libraries(QmlTuiCore PRIVATE
    utf8proc
)
# A static QmlTui (WebAssembly, static Qt, or BUILD_SHARED_LIBS=OFF) carries
# utf8proc in its link interface and needs its static QML plugin linked into
# consumers. Shared builds keep utf8proc private and load the plugin at runtime.
get_target_property(qmltui_core_library_type QmlTuiCore TYPE)
if(qmltui_core_library_type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(QmlTuiCore PUBLIC QMLTUI_CORE_STATIC)
    target_link_libraries(QmlTuiCore INTERFACE QmlTuiCoreplugin)
    if(NOT QMLTUI_USE_SYSTEM_UTF8PROC)
        install(TARGETS utf8proc
            EXPORT QmlTuiTargets
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        )
    endif()
else()
    target_compile_definitions(QmlTuiCore PRIVATE QMLTUI_CORE_LIBRARY)
endif()
if(QMLTUI_ENABLE_PRIVATE_QT)
    target_link_libraries(QmlTuiCore PRIVATE Qt6::QmlPrivate)
    target_compile_definitions(QmlTuiCore PRIVATE QMLTUI_ENABLE_PRIVATE_QT)
endif()

add_library(qmltui::core ALIAS QmlTuiCore)
add_library(QmlTui::Core ALIAS QmlTuiCore)
set_target_properties(QmlTuiCore PROPERTIES EXPORT_NAME Core)
file(RELATIVE_PATH _qmltui_rel_libdir
    "${CMAKE_INSTALL_PREFIX}/${QMLTUI_INSTALL_QMLDIR}/QmlTui"
    "${CMAKE_INSTALL_FULL_LIBDIR}"
)
set_target_properties(QmlTuiCoreplugin PROPERTIES
    EXPORT_NAME CorePlugin
    INSTALL_RPATH "$ORIGIN;$ORIGIN/${_qmltui_rel_libdir}"
)
install(TARGETS QmlTuiCore
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QmlTuiCoreplugin
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui"
    RUNTIME DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui"
    ARCHIVE DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui"
)
# Static builds (WebAssembly) add resource and plugin-init object libraries to
# the module's link interface; the export set must carry them too. Empty on
# shared builds.
if(qmltui_core_output_targets)
    install(TARGETS ${qmltui_core_output_targets}
        EXPORT QmlTuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/QmlTuiCore.qmltypes"
    DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui"
)
# The public C++ surface. Everything a listed header includes must be listed
# too; qmltui_check_public_headers() enforces it at configure time.
qmltui_install_headers(QmlTui/Core/Items FILES
    Items/AnchorLine.h Items/Anchors.h Items/Border.h Items/Canvas.h Items/Column.h
    Items/FocusScope.h Items/Gradient.h Items/GradientStop.h Items/GridLayout.h Items/GridView.h
    Items/Item.h Items/Layout.h Items/LinearLayout.h Items/ListView.h Items/Loader.h
    Items/MouseArea.h Items/Popup.h Items/Rectangle.h Items/Repeater.h Items/Row.h
    Items/Scrollback.h Items/ScrollView.h Items/StackLayout.h Items/TableView.h
    Items/Terminal.h Items/Text.h Items/TextArea.h Items/TextInput.h Items/TreeView.h
)
qmltui_install_headers(QmlTui/Core/Primitives FILES
    Primitives/Span.h Primitives/TextStyle.h Primitives/HighlightRange.h Primitives/QmlTuiGlobal.h
    Primitives/QmlTuiExport.h Primitives/Color.h Primitives/TextGridBuffer.h Primitives/Cell.h
    Primitives/UnicodeText.h Primitives/Subpixel.h Primitives/TerminalCapabilities.h
    # The application-provided backend extension point (TerminalBackend::setFactory).
    Primitives/TerminalBackend.h
)
qmltui_install_headers(QmlTui/Core/Engine FILES
    Engine/Animation.h Engine/Clipboard.h
    Engine/KeyEvent.h Engine/Keys.h Engine/MouseEvent.h
    # Popup implements this interface.
    Engine/InputScope.h
    Engine/QtMessageRouting.h Engine/Shortcut.h
    Engine/State.h Engine/StateGroup.h Engine/Transition.h
)
