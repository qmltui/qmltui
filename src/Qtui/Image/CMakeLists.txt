qt_add_qml_module(QtuiImage
    URI Qtui.Image
    OUTPUT_TARGETS qtui_image_output_targets
    SOURCES
        Image.h
        Image.cpp
        ImageProvider.h
        ImageProvider.cpp
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

target_link_libraries(QtuiImage PUBLIC Qtui::Core Qt6::Gui PRIVATE Qt6::Core Qt6::Qml Qt6::Network)

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
if(qtui_image_output_targets)   # static builds only, see Core
    install(TARGETS ${qtui_image_output_targets}
        EXPORT QtuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Image/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Image/QtuiImage.qmltypes"
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Image"
)
qtui_install_headers(Qtui/Image FILES Image.h ImageProvider.h TerminalGraphics.h)
