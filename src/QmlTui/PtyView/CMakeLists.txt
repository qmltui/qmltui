# The embedded emulator comes from libvterm, system or fetched; see
# cmake/QmlTuiLibvterm.cmake. Only this module needs it, so only this module asks.
qmltui_provide_libvterm()

qt_add_qml_module(QmlTuiPtyView
    URI QmlTui.PtyView
    ${QMLTUI_STATIC_QML_MODULE}
    OUTPUT_TARGETS qmltui_pty_view_output_targets
    SOURCES
        PtyView.h
        PtyView.cpp
        VTermAdapter.h
        VTermAdapter.cpp
        UnixPtySession.h
        UnixPtySession.cpp
    DEPENDENCIES
        TARGET QmlTuiCore
)

target_include_directories(QmlTuiPtyView
    PUBLIC
        $<BUILD_INTERFACE:${qmltui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)

target_link_libraries(QmlTuiPtyView
    PUBLIC QmlTui::Core
    PRIVATE qmltui_libvterm
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(QmlTuiPtyView PRIVATE util)
endif()

get_target_property(qmltui_pty_view_library_type QmlTuiPtyView TYPE)
if(qmltui_pty_view_library_type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(QmlTuiPtyView PUBLIC QMLTUI_PTYVIEW_STATIC)
    target_link_libraries(QmlTuiPtyView INTERFACE QmlTuiPtyViewplugin)
else()
    target_compile_definitions(QmlTuiPtyView PRIVATE QMLTUI_PTYVIEW_LIBRARY)
endif()
add_library(QmlTui::PtyView ALIAS QmlTuiPtyView)
set_target_properties(QmlTuiPtyView PROPERTIES EXPORT_NAME PtyView)
set_target_properties(QmlTuiPtyViewplugin PROPERTIES EXPORT_NAME PtyViewPlugin)
qmltui_set_install_rpath(QmlTuiPtyView "${CMAKE_INSTALL_LIBDIR}")
qmltui_set_install_rpath(QmlTuiPtyViewplugin "${QMLTUI_INSTALL_QMLDIR}/QmlTui/PtyView")

install(TARGETS QmlTuiPtyView
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QmlTuiPtyViewplugin
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/PtyView"
    RUNTIME DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/PtyView"
    ARCHIVE DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/PtyView"
)
if(qmltui_pty_view_output_targets)   # static builds only, see Core
    install(TARGETS ${qmltui_pty_view_output_targets}
        EXPORT QmlTuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
if(qmltui_pty_view_library_type STREQUAL "STATIC_LIBRARY" AND TARGET qmltui_libvterm AND NOT QMLTUI_USE_SYSTEM_LIBVTERM)
    install(TARGETS qmltui_libvterm
        EXPORT QmlTuiTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/PtyView/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/PtyView/QmlTuiPtyView.qmltypes"
    DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/PtyView"
)
qmltui_install_headers(QmlTui/PtyView FILES PtyView.h)
install(FILES "${PROJECT_SOURCE_DIR}/docs/third-party/libvterm-LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/QmlTui/third-party"
)
