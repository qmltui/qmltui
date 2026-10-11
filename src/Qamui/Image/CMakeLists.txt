qt_add_qml_module(QamuiImage
    URI Qamui.Image
    OUTPUT_TARGETS qamui_image_output_targets
    SOURCES
        AnimatedSprite.h
        AnimatedSprite.cpp
        Image.h
        Image.cpp
        ImageProvider.h
        ImageProvider.cpp
        TerminalGraphics.h
        TerminalGraphics.cpp
    DEPENDENCIES TARGET QamuiCore
)

target_compile_definitions(QamuiImage PRIVATE QAMUI_IMAGE_LIBRARY)
add_library(Qamui::Image ALIAS QamuiImage)

target_include_directories(QamuiImage
    PUBLIC
        $<BUILD_INTERFACE:${Qamui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)

target_link_libraries(QamuiImage PUBLIC Qamui::Core Qt6::Gui PRIVATE Qt6::Core Qt6::Qml Qt6::Network)

set_target_properties(QamuiImage PROPERTIES EXPORT_NAME Image)
set_target_properties(QamuiImageplugin PROPERTIES EXPORT_NAME ImagePlugin)
set_target_properties(QamuiImage QamuiImageplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../../.."
)

install(TARGETS QamuiImage
    EXPORT QamuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QamuiImageplugin
    EXPORT QamuiTargets
    LIBRARY DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/Image"
    RUNTIME DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/Image"
)
if(qamui_image_output_targets)   # static builds only, see Core
    install(TARGETS ${qamui_image_output_targets}
        EXPORT QamuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qamui/Image/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qamui/Image/QamuiImage.qmltypes"
    DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/Image"
)
qamui_install_headers(Qamui/Image FILES AnimatedSprite.h Image.h ImageProvider.h TerminalGraphics.h)
