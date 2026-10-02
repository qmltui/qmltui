qt_add_qml_module(QtuiCanvas
    URI Qtui.Canvas
    SOURCES
        Canvas.h
        Canvas.cpp
    DEPENDENCIES TARGET QtuiCore
)

add_library(Qtui::Canvas ALIAS QtuiCanvas)
target_include_directories(QtuiCanvas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/canvas>
)
target_link_libraries(QtuiCanvas PUBLIC Qtui::Core PRIVATE Qt6::Core Qt6::Qml)
set_target_properties(QtuiCanvas PROPERTIES EXPORT_NAME Canvas)
set_target_properties(QtuiCanvasplugin PROPERTIES EXPORT_NAME CanvasPlugin)
set_target_properties(QtuiCanvas QtuiCanvasplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../../.."
)

install(TARGETS QtuiCanvas QtuiCanvasplugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Canvas"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Canvas"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Canvas/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Canvas/QtuiCanvas.qmltypes"
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Canvas"
)
install(FILES
    Canvas.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/canvas"
)
