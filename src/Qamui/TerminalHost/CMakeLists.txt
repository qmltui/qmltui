# The embedded emulator comes from libvterm, system or fetched; see
# cmake/QamuiLibvterm.cmake. Only this module needs it, so only this module asks.
qamui_provide_libvterm()

qt_add_qml_module(QamuiTerminalHost
    URI Qamui.TerminalHost
    SOURCES
        TerminalView.h
        TerminalView.cpp
        VTermAdapter.h
        VTermAdapter.cpp
        UnixPtySession.h
        UnixPtySession.cpp
    DEPENDENCIES
        TARGET QamuiCore
)

target_include_directories(QamuiTerminalHost
    PUBLIC
        $<BUILD_INTERFACE:${Qamui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)

target_link_libraries(QamuiTerminalHost
    PUBLIC QamuiCore
    PRIVATE qamui_libvterm
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(QamuiTerminalHost PRIVATE util)
endif()

target_compile_definitions(QamuiTerminalHost PRIVATE QAMUI_TERMINALHOST_LIBRARY)
add_library(Qamui::TerminalHost ALIAS QamuiTerminalHost)
set_target_properties(QamuiTerminalHost PROPERTIES EXPORT_NAME TerminalHost)
set_target_properties(QamuiTerminalHost QamuiTerminalHostplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../.."
)
set_target_properties(QamuiTerminalHostplugin PROPERTIES EXPORT_NAME TerminalHostPlugin)

install(TARGETS QamuiTerminalHost QamuiTerminalHostplugin
    EXPORT QamuiTargets
    LIBRARY DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/TerminalHost"
    RUNTIME DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/TerminalHost"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qamui/TerminalHost/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qamui/TerminalHost/QamuiTerminalHost.qmltypes"
    DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/TerminalHost"
)
qamui_install_headers(Qamui/TerminalHost FILES TerminalView.h)
install(FILES "${PROJECT_SOURCE_DIR}/docs/third-party/libvterm-LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/Qamui/third-party"
)
