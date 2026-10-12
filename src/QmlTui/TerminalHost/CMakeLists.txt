# The embedded emulator comes from libvterm, system or fetched; see
# cmake/QmlTuiLibvterm.cmake. Only this module needs it, so only this module asks.
qmltui_provide_libvterm()

qt_add_qml_module(QmlTuiTerminalHost
    URI QmlTui.TerminalHost
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
set_target_properties(QmlTuiTerminalHost QmlTuiTerminalHostplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../.."
)
set_target_properties(QmlTuiTerminalHostplugin PROPERTIES EXPORT_NAME TerminalHostPlugin)

install(TARGETS QmlTuiTerminalHost QmlTuiTerminalHostplugin
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/TerminalHost"
    RUNTIME DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/TerminalHost"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/TerminalHost/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/TerminalHost/QmlTuiTerminalHost.qmltypes"
    DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/TerminalHost"
)
qmltui_install_headers(QmlTui/TerminalHost FILES TerminalView.h)
install(FILES "${PROJECT_SOURCE_DIR}/docs/third-party/libvterm-LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/QmlTui/third-party"
)
