qt_add_qml_module(QtuiImage
    URI Qtui.Image
    SOURCES
        Image.h
        Image.cpp
    DEPENDENCIES TARGET QtuiCore
)

add_library(Qtui::Image ALIAS QtuiImage)

target_include_directories(QtuiImage PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/image>
)

target_link_libraries(QtuiImage PUBLIC Qtui::Core Qt6::Gui PRIVATE Qt6::Core Qt6::Qml)

set_target_properties(QtuiImage PROPERTIES EXPORT_NAME Image)
set_target_properties(QtuiImageplugin PROPERTIES EXPORT_NAME ImagePlugin)
set_target_properties(QtuiImage QtuiImageplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../../.."
)

install(TARGETS QtuiImage QtuiImageplugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Image"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Image"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Image/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Image/QtuiImage.qmltypes"
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Image"
)
install(FILES
    Image.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/image"
)
