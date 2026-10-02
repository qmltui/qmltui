

# The library backing the QML module
set(qtui_behavior_sources)
if(QTUI_ENABLE_BEHAVIOR)
    list(APPEND qtui_behavior_sources
        src/engine/Behavior.h
        src/engine/Behavior.cpp
    )
endif()

qt_add_qml_module(QtuiCore
    URI Qtui
    PLUGIN_TARGET QtuiCorePlugin
    SOURCES
        src/core/ColorMapper.h
        src/core/ColorMapper.cpp
        src/core/InputDecoder.cpp
        src/core/Cell.h
        src/core/UnicodeText.h
        src/core/UnicodeText.cpp
        src/core/TextGridBuffer.h
        src/core/TextGridBuffer.cpp
        src/core/TerminalDevice.h
        src/core/TerminalDevice.cpp
        src/engine/TerminalEngine.h
        src/engine/TerminalEngine.cpp
        src/engine/Renderer.h
        src/engine/FrameRenderer.h
        src/engine/FrameRenderer.cpp
        src/engine/StreamRenderer.h
        src/engine/StreamRenderer.cpp
        src/engine/Shortcut.h
        src/engine/Shortcut.cpp
        src/engine/KeyEvent.h
        src/engine/MouseEvent.h
        src/engine/Keys.h
        src/engine/Animation.h
        src/engine/Animation.cpp
        src/engine/State.h
        src/engine/State.cpp
        src/engine/Transition.h
        src/engine/Transition.cpp
        src/items/Item.h
        src/items/Item.cpp
        src/items/Rectangle.h
        src/items/Rectangle.cpp
        src/items/Anchors.h
        src/items/Anchors.cpp
        src/items/AnchorLine.h
        src/items/Border.h
        src/items/Border.cpp
        src/items/TextInput.h
        src/items/TextInput.cpp
        src/items/TextArea.h
        src/items/TextArea.cpp
        src/items/LogView.h
        src/items/LogView.cpp
        src/items/ScrollView.h
        src/items/ScrollView.cpp
        src/items/Terminal.h
        src/items/Terminal.cpp
        src/items/Popup.h
        src/items/Popup.cpp
        src/items/Text.h
        src/items/Text.cpp
        src/items/StreamAppender.h
        src/items/StreamAppender.cpp
        src/items/Row.h
        src/items/Row.cpp
        src/items/Column.h
        src/items/Column.cpp
        src/items/Layout.h
        src/items/Layout.cpp
        src/items/LinearLayout.h
        src/items/LinearLayout.cpp
        src/items/MouseArea.h
        src/items/MouseArea.cpp
        src/items/Repeater.h
        src/items/Repeater.cpp
        src/items/ListView.h
        src/items/ListView.cpp
        src/items/GridView.h
        src/items/GridView.cpp
        src/items/BrailleCanvas.h
        src/items/BrailleCanvas.cpp
        ${qtui_behavior_sources}
)

target_include_directories(QtuiCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/items>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/engine>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/items>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/core>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/engine>
)

target_link_libraries(QtuiCore PUBLIC
    Qt6::Qml
)

target_link_libraries(QtuiCore PRIVATE
    utf8proc
)
if(QTUI_ENABLE_BEHAVIOR)
    target_link_libraries(QtuiCore PRIVATE Qt6::QmlPrivate)
endif()

add_library(Qtui::Core ALIAS QtuiCore)
set_target_properties(QtuiCore PROPERTIES EXPORT_NAME Core)
set_target_properties(QtuiCorePlugin PROPERTIES
    EXPORT_NAME CorePlugin
    INSTALL_RPATH "$ORIGIN/../../.."
)
install(TARGETS QtuiCore
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QtuiCorePlugin
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
    src/items/Item.h src/items/Anchors.h src/items/AnchorLine.h src/items/Border.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/items"
)
install(FILES
    src/core/TextGridBuffer.h src/core/Cell.h src/core/UnicodeText.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/core"
)
install(FILES
    src/engine/KeyEvent.h src/engine/MouseEvent.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/engine"
)

add_subdirectory(Controls)
add_subdirectory(Widgets)
