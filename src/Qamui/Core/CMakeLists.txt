include("${Qamui_SOURCE_DIR}/cmake/CheckEngineIncludes.cmake")

# The library backing the QML module
set(qamui_behavior_sources)
if(QAMUI_ENABLE_BEHAVIOR)
    list(APPEND qamui_behavior_sources
        engine/Behavior.h
        engine/Behavior.cpp
    )
endif()

# One terminal backend per platform behind primitives/TerminalBackend.h.
set(qamui_platform_sources)
if(WIN32)
    list(APPEND qamui_platform_sources
        primitives/Win32ConsoleBackend.h
        primitives/Win32ConsoleBackend.cpp
    )
elseif(EMSCRIPTEN)
    list(APPEND qamui_platform_sources
        primitives/WasmTerminalBackend.h
        primitives/WasmTerminalBackend.cpp
    )
else()
    list(APPEND qamui_platform_sources
        primitives/PosixTerminalBackend.h
        primitives/PosixTerminalBackend.cpp
    )
endif()

qt_add_qml_module(QamuiCore
    URI Qamui
    OUTPUT_TARGETS qamui_core_output_targets
    SOURCES
        primitives/Span.h
        primitives/Span.cpp
        primitives/Color.h
        primitives/Color.cpp
        primitives/QamuiExport.h
        primitives/QamuiGlobal.h
        primitives/QamuiGlobal.cpp
        primitives/ColorMapper.h
        primitives/ColorMapper.cpp
        primitives/InputDecoder.cpp
        primitives/Cell.h
        primitives/Cell.cpp
        primitives/UnicodeText.h
        primitives/UnicodeText.cpp
        primitives/TextGridBuffer.h
        primitives/TextGridBuffer.cpp
        primitives/TextGridBufferAnsi.cpp
        primitives/AnsiEncoder_p.h
        primitives/TerminalCapabilities.h
        primitives/TerminalCapabilities.cpp
        primitives/TerminalBackend.h
        primitives/TerminalBackend.cpp
        ${qamui_platform_sources}
        primitives/TerminalDevice.h
        primitives/TerminalDevice.cpp
        engine/Clock_p.h
        engine/Clock.cpp
        engine/TerminalEngine.h
        engine/TerminalEngine.cpp
        engine/TerminalMessageRouter_p.h
        engine/TerminalMessageRouter.cpp
        engine/InputRouter_p.h
        engine/InputRouter.cpp
        engine/InputScope.h
        engine/FocusManager.h
        engine/FocusManager.cpp
        engine/Renderer.h
        engine/RenderPhase_p.h
        engine/RenderPhase.cpp
        engine/ItemPaintTraversal_p.h
        engine/FrameRenderer.h
        engine/FrameRenderer.cpp
        engine/StreamRenderer.h
        engine/StreamRenderer.cpp
        engine/Shortcut.h
        engine/Shortcut.cpp
        engine/KeyEvent.h
        engine/MouseEvent.h
        engine/Keys.h
        engine/Keys.cpp
        engine/Animation.h
        engine/Animation.cpp
        engine/State.h
        engine/State.cpp
        engine/Transition.h
        engine/Transition.cpp
        items/Item.h
        items/Item.cpp
        items/Canvas.h
        items/Canvas.cpp
        items/Rectangle.h
        items/Rectangle.cpp
        items/Anchors.h
        items/Anchors.cpp
        items/AnchorLine.h
        items/Border.h
        items/Border.cpp
        items/TextInput.h
        items/TextInput.cpp
        items/TextEditorState.h
        items/TextEditorState.cpp
        items/TextArea.h
        items/TextArea.cpp
        items/LogView.h
        items/LogView.cpp
        items/ScrollView.h
        items/ScrollView.cpp
        items/GridLayout.h
        items/GridLayout.cpp
        items/Terminal.h
        items/Terminal.cpp
        items/Popup.h
        items/Popup.cpp
        items/Text.h
        items/Text.cpp
        items/GradientStop.h
        items/GradientStop.cpp
        items/Gradient.h
        items/Gradient.cpp
        items/Scrollback.h
        items/Scrollback.cpp
        items/DelegateUtils.h
        items/NumericItemRoles_p.h
        items/Row.h
        items/Row.cpp
        items/Column.h
        items/Column.cpp
        items/Layout.h
        items/Layout.cpp
        items/LinearLayout.h
        items/LinearLayout.cpp
        items/MouseArea.h
        items/MouseArea.cpp
        items/Repeater.h
        items/Repeater.cpp
        items/Loader.h
        items/Loader.cpp
        items/ListView.h
        items/ListView.cpp
        items/GridView.h
        items/GridView.cpp
        items/Paragraph.h
        items/Paragraph.cpp
        items/TableView.h
        items/TableView.cpp
        items/TreeView.h
        items/TreeView.cpp
        ${qamui_behavior_sources}
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
        ${CMAKE_CURRENT_SOURCE_DIR}/primitives
        ${CMAKE_CURRENT_SOURCE_DIR}/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/items
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
if(QAMUI_ENABLE_BEHAVIOR)
    target_link_libraries(QamuiCore PRIVATE Qt6::QmlPrivate)
    target_compile_definitions(QamuiCore PRIVATE QAMUI_ENABLE_BEHAVIOR)
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
qamui_install_headers(Qamui/Core/items FILES
    items/Text.h
    items/Item.h items/Anchors.h items/AnchorLine.h items/Border.h items/Paragraph.h
    items/TableView.h items/TreeView.h items/Terminal.h items/Rectangle.h
    items/Gradient.h items/GradientStop.h items/Canvas.h items/Popup.h
)
qamui_install_headers(Qamui/Core/primitives FILES
    primitives/Span.h primitives/QamuiGlobal.h
    primitives/QamuiExport.h primitives/Color.h primitives/TextGridBuffer.h primitives/Cell.h
    primitives/UnicodeText.h primitives/TerminalCapabilities.h
    # The application-provided backend extension point (TerminalBackend::setFactory).
    primitives/TerminalBackend.h
    # Pulled in by the installed item headers, so it has to ship with them.
    primitives/ColorMapper.h
)
qamui_install_headers(Qamui/Core/engine FILES
    engine/KeyEvent.h engine/MouseEvent.h
    # Popup implements this interface.
    engine/InputScope.h
)
