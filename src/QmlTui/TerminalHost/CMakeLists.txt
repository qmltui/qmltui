# The embedded emulator comes from libvterm, system or fetched; see
# cmake/QmlTuiLibvterm.cmake. Only this module needs it, so only this module asks.
qmltui_provide_libvterm()

qt_add_qml_module(QmlTuiTerminalHost
    URI QmlTui.TerminalHost
    OUTPUT_TARGETS qmltui_terminal_host_output_targets
    SOURCES
        TerminalView.h
        TerminalView.cpp
        VTermAdapter.h
        VTermAdapter.cpp
        UnixPtySession.h
        UnixPtySession.cpp
    DEPENDENCIES
        TARGET QmlTuiCore
)

target_include_directories(QmlTuiTerminalHost
    PUBLIC
        $<BUILD_INTERFACE:${qmltui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)

target_link_libraries(QmlTuiTerminalHost
    PUBLIC QmlTuiCore
    PRIVATE qmltui_libvterm
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(QmlTuiTerminalHost PRIVATE util)
endif()

target_compile_definitions(QmlTuiTerminalHost PRIVATE QMLTUI_TERMINALHOST_LIBRARY)
add_library(qmltui::terminal_host ALIAS QmlTuiTerminalHost)
add_library(QmlTui::TerminalHost ALIAS QmlTuiTerminalHost)
set_target_properties(QmlTuiTerminalHost PROPERTIES EXPORT_NAME TerminalHost)
set_target_properties(QmlTuiTerminalHostplugin PROPERTIES EXPORT_NAME TerminalHostPlugin)
file(RELATIVE_PATH _qmltui_rel_libdir
    "${CMAKE_INSTALL_PREFIX}/${QMLTUI_INSTALL_QMLDIR}/QmlTui/TerminalHost"
    "${CMAKE_INSTALL_FULL_LIBDIR}"
)
set_target_properties(QmlTuiTerminalHost QmlTuiTerminalHostplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/${_qmltui_rel_libdir}"
)

install(TARGETS QmlTuiTerminalHost
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QmlTuiTerminalHostplugin
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/TerminalHost"
    RUNTIME DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/TerminalHost"
)
if(qmltui_terminal_host_output_targets)   # static builds only, see Core
    install(TARGETS ${qmltui_terminal_host_output_targets}
        EXPORT QmlTuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
get_target_property(qmltui_terminal_host_library_type QmlTuiTerminalHost TYPE)
if(qmltui_terminal_host_library_type STREQUAL "STATIC_LIBRARY" AND TARGET qmltui_libvterm AND NOT QMLTUI_USE_SYSTEM_LIBVTERM)
    install(TARGETS qmltui_libvterm
        EXPORT QmlTuiTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/TerminalHost/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/TerminalHost/QmlTuiTerminalHost.qmltypes"
    DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/TerminalHost"
)
qmltui_install_headers(QmlTui/TerminalHost FILES TerminalView.h)
install(FILES "${PROJECT_SOURCE_DIR}/docs/third-party/libvterm-LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/QmlTui/third-party"
)
