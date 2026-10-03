


# The library backing the QML module
set(qtui_behavior_sources)
if(QTUI_ENABLE_BEHAVIOR)
    list(APPEND qtui_behavior_sources
        engine/Behavior.h
        engine/Behavior.cpp
    )
endif()

qt_add_qml_module(QtuiCore
    URI Qtui
    SOURCES
        primitives/ColorMapper.h
        primitives/ColorMapper.cpp
        primitives/InputDecoder.cpp
        primitives/Cell.h
        primitives/UnicodeText.h
        primitives/UnicodeText.cpp
        primitives/TextGridBuffer.h
        primitives/TextGridBuffer.cpp
        primitives/TerminalDevice.h
        primitives/TerminalDevice.cpp
        engine/TerminalEngine.h
        engine/TerminalEngine.cpp
        engine/TestDriver.h
        engine/TestDriver.cpp
        engine/Renderer.h
        engine/FrameRenderer.h
        engine/FrameRenderer.cpp
        engine/StreamRenderer.h
        engine/StreamRenderer.cpp
        engine/Shortcut.h
        engine/Shortcut.cpp
        engine/KeyEvent.h
        engine/MouseEvent.h
        engine/Keys.h
        engine/Animation.h
        engine/Animation.cpp
        engine/State.h
        engine/State.cpp
        engine/Transition.h
        engine/Transition.cpp
        items/Item.h
        items/Item.cpp
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
        items/StreamAppender.h
        items/StreamAppender.cpp
        items/DelegateUtils.h
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
        ${qtui_behavior_sources}
)

target_include_directories(QtuiCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/items>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/primitives>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/items>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/primitives>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/engine>
)

target_link_libraries(QtuiCore PUBLIC
    Qt6::Qml
)

target_link_libraries(QtuiCore PRIVATE
    utf8proc
)
if(QTUI_ENABLE_TEST_DRIVER)
    find_package(Qt6 REQUIRED COMPONENTS Gui)
    target_link_libraries(QtuiCore PRIVATE Qt6::Gui)
endif()
if(QTUI_ENABLE_BEHAVIOR)
    target_link_libraries(QtuiCore PRIVATE Qt6::QmlPrivate)
endif()

add_library(Qtui::Core ALIAS QtuiCore)
set_target_properties(QtuiCore PROPERTIES EXPORT_NAME Core)
set_target_properties(QtuiCoreplugin PROPERTIES
    EXPORT_NAME CorePlugin
    INSTALL_RPATH "$ORIGIN/../../.."
)
install(TARGETS QtuiCore
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QtuiCoreplugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui"
)

install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/QtuiCore.qmltypes"
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui"
)
install(FILES
    items/Item.h items/Anchors.h items/AnchorLine.h items/Border.h items/Paragraph.h items/TableView.h items/TreeView.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/items"
)
install(FILES
    primitives/TextGridBuffer.h primitives/Cell.h primitives/UnicodeText.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/primitives"
)
install(FILES
    engine/KeyEvent.h engine/MouseEvent.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/engine"
)


add_subdirectory(Canvas)
add_subdirectory(Controls)
add_subdirectory(Extras)
if(QTUI_BUILD_TERMINAL_HOST)
    add_subdirectory(TerminalHost)
endif()
