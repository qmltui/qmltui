qt_add_qml_module(QtuiImage
    URI Qtui.Image
    SOURCES
        Image.h
        Image.cpp
        TerminalGraphics.h
        TerminalGraphics.cpp
    DEPENDENCIES TARGET QtuiCore
)

target_compile_definitions(QtuiImage PRIVATE QTUI_IMAGE_LIBRARY)
add_library(Qtui::Image ALIAS QtuiImage)

target_include_directories(QtuiImage
    PUBLIC
        $<BUILD_INTERFACE:${Qtui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)

target_link_libraries(QtuiImage PUBLIC Qtui::Core Qt6::Gui PRIVATE Qt6::Core Qt6::Qml)

set_target_properties(QtuiImage PROPERTIES EXPORT_NAME Image)
set_target_properties(QtuiImageplugin PROPERTIES EXPORT_NAME ImagePlugin)
set_target_properties(QtuiImage QtuiImageplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../../.."
)

install(TARGETS QtuiImage
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QtuiImageplugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Image"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Image"
)
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Image/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Image/QtuiImage.qmltypes"
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Image"
)
qtui_install_headers(Qtui/Image FILES Image.h TerminalGraphics.h)
