include("${Qamui_SOURCE_DIR}/cmake/CheckEngineIncludes.cmake")

set(qamui_private_qt_sources)
if(QAMUI_ENABLE_PRIVATE_QT)
    list(APPEND qamui_private_qt_sources
        Engine/Behavior.h
        Engine/Behavior.cpp
        Engine/StateGroup.h
        Engine/StateGroup.cpp
    )
endif()

# One terminal backend per platform behind Primitives/TerminalBackend.h.
set(qamui_platform_sources)
if(WIN32)
    list(APPEND qamui_platform_sources
        Primitives/Win32ConsoleBackend.h
        Primitives/Win32ConsoleBackend.cpp
    )
elseif(EMSCRIPTEN)
    list(APPEND qamui_platform_sources
        Primitives/WasmTerminalBackend.h
        Primitives/WasmTerminalBackend.cpp
    )
else()
    list(APPEND qamui_platform_sources
        Primitives/PosixTerminalBackend.h
        Primitives/PosixTerminalBackend.cpp
    )
endif()

qt_add_qml_module(QamuiCore
    URI Qamui
    OUTPUT_TARGETS qamui_core_output_targets
    SOURCES
        Primitives/Span.h
        Primitives/Span.cpp
        Primitives/TextStyle.h
        Primitives/TextStyle.cpp
        Primitives/HighlightRange.h
        Primitives/HighlightRange.cpp
        Primitives/Color.h
        Primitives/Color.cpp
        Primitives/QamuiExport.h
        Primitives/QamuiGlobal.h
        Primitives/QamuiGlobal.cpp
        Primitives/ColorMapper.h
        Primitives/ColorMapper.cpp
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
        ${qamui_platform_sources}
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
        Engine/FrameRenderer.h
        Engine/FrameRenderer.cpp
        Engine/StreamRenderer.h
        Engine/StreamRenderer.cpp
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
        Engine/Transition.h
        Engine/Transition.cpp
        Engine/StateController_p.h
        Engine/StateController.cpp
        ${qamui_private_qt_sources}
        Items/Item.h
        Items/Item.cpp
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
        Items/LogView.h
        Items/LogView.cpp
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

# Headers are reached as <Qamui/Core/<layer>/Name.h> from the src/ root in the
# build tree and from the include root once installed. The private entries
# serve the generated QML type registrations, which include each registered
# header by its bare file name and need its directory on the include path.
target_include_directories(QamuiCore
    PUBLIC
        $<BUILD_INTERFACE:${Qamui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Primitives
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine
        ${CMAKE_CURRENT_SOURCE_DIR}/Items
)

target_link_libraries(QamuiCore PUBLIC
    Qt6::Qml
)

target_link_libraries(QamuiCore PRIVATE
    utf8proc
)
# A static Qamui (WebAssembly) carries utf8proc in its link interface, so the
# export set must include it. Shared builds keep it private.
get_target_property(qamui_core_library_type QamuiCore TYPE)
if(qamui_core_library_type STREQUAL "STATIC_LIBRARY" AND NOT QAMUI_USE_SYSTEM_UTF8PROC)
    install(TARGETS utf8proc
        EXPORT QamuiTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
if(QAMUI_ENABLE_PRIVATE_QT)
    target_link_libraries(QamuiCore PRIVATE Qt6::QmlPrivate)
    target_compile_definitions(QamuiCore PUBLIC
        QAMUI_ENABLE_PRIVATE_QT
        QAMUI_HAS_QML_PRIVATE
        QAMUI_ENABLE_BEHAVIOR
        QAMUI_ENABLE_ADVANCED_STATES
    )
endif()

target_compile_definitions(QamuiCore PRIVATE QAMUI_CORE_LIBRARY)
add_library(Qamui::Core ALIAS QamuiCore)
set_target_properties(QamuiCore PROPERTIES EXPORT_NAME Core)
set_target_properties(QamuiCoreplugin PROPERTIES
    EXPORT_NAME CorePlugin
    INSTALL_RPATH "$ORIGIN/../../.."
)
install(TARGETS QamuiCore
    EXPORT QamuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QamuiCoreplugin
    EXPORT QamuiTargets
    LIBRARY DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui"
    RUNTIME DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui"
)
# Static builds (WebAssembly) add resource and plugin-init object libraries to
# the module's link interface; the export set must carry them too. Empty on
# shared builds.
if(qamui_core_output_targets)
    install(TARGETS ${qamui_core_output_targets}
        EXPORT QamuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qamui/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qamui/QamuiCore.qmltypes"
    DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui"
)
# The public C++ surface. Everything a listed header includes must be listed
# too; qamui_check_public_headers() enforces it at configure time.
qamui_install_headers(Qamui/Core/Items FILES
    Items/Text.h
    Items/Item.h Items/Anchors.h Items/AnchorLine.h Items/Border.h
    Items/TableView.h Items/TreeView.h Items/Terminal.h Items/Rectangle.h
    Items/Gradient.h Items/GradientStop.h Items/Canvas.h Items/Popup.h
    Items/TextArea.h Items/TextEditorState.h
)
qamui_install_headers(Qamui/Core/Primitives FILES
    Primitives/Span.h Primitives/TextStyle.h Primitives/HighlightRange.h Primitives/QamuiGlobal.h
    Primitives/QamuiExport.h Primitives/Color.h Primitives/TextGridBuffer.h Primitives/Cell.h
    Primitives/UnicodeText.h Primitives/Subpixel.h Primitives/TerminalCapabilities.h
    # The application-provided backend extension point (TerminalBackend::setFactory).
    Primitives/TerminalBackend.h
    # Pulled in by the installed item headers, so it has to ship with them.
    Primitives/ColorMapper.h
)
qamui_install_headers(Qamui/Core/Engine FILES
    Engine/KeyEvent.h Engine/MouseEvent.h
    # Popup implements this interface.
    Engine/InputScope.h
    Engine/QtMessageRouting.h
)
