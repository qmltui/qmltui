qt_add_qml_module(QmlTuiImage
    URI QmlTui.Image
    OUTPUT_TARGETS qmltui_image_output_targets
    SOURCES
        AnimatedSprite.h
        AnimatedSprite.cpp
        Image.h
        Image.cpp
        ImageProvider.h
        ImageProvider.cpp
        TerminalGraphics.h
        TerminalGraphics.cpp
    DEPENDENCIES TARGET QmlTuiCore
)

target_compile_definitions(QmlTuiImage PRIVATE QMLTUI_IMAGE_LIBRARY)
add_library(qmltui::image ALIAS QmlTuiImage)
add_library(QmlTui::Image ALIAS QmlTuiImage)

target_include_directories(QmlTuiImage
    PUBLIC
        $<BUILD_INTERFACE:${qmltui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)

target_link_libraries(QmlTuiImage PUBLIC qmltui::core Qt6::Gui PRIVATE Qt6::Core Qt6::Qml Qt6::Network)

set_target_properties(QmlTuiImage PROPERTIES EXPORT_NAME Image)
set_target_properties(QmlTuiImageplugin PROPERTIES EXPORT_NAME ImagePlugin)
set_target_properties(QmlTuiImage QmlTuiImageplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../../.."
)

install(TARGETS QmlTuiImage
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QmlTuiImageplugin
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/Image"
    RUNTIME DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/Image"
)
if(qmltui_image_output_targets)   # static builds only, see Core
    install(TARGETS ${qmltui_image_output_targets}
        EXPORT QmlTuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/Image/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/Image/QmlTuiImage.qmltypes"
    DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/Image"
)
qmltui_install_headers(QmlTui/Image FILES AnimatedSprite.h Image.h ImageProvider.h TerminalGraphics.h)
