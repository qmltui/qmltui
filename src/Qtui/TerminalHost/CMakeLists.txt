qt_add_qml_module(QtuiTerminalHost
    URI Qtui.TerminalHost
    SOURCES
        TerminalView.h
        TerminalView.cpp
        VTermAdapter.h
        VTermAdapter.cpp
        UnixPtySession.h
        UnixPtySession.cpp
    DEPENDENCIES
        TARGET QtuiCore
)

target_include_directories(QtuiTerminalHost PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/terminalhost>
)

target_link_libraries(QtuiTerminalHost
    PUBLIC QtuiCore
    PRIVATE qtui_libvterm
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(QtuiTerminalHost PRIVATE util)
endif()

target_compile_definitions(QtuiTerminalHost PRIVATE QTUI_TERMINALHOST_LIBRARY)
add_library(Qtui::TerminalHost ALIAS QtuiTerminalHost)
set_target_properties(QtuiTerminalHost PROPERTIES EXPORT_NAME TerminalHost)
set_target_properties(QtuiTerminalHost QtuiTerminalHostplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../.."
)
set_target_properties(QtuiTerminalHostplugin PROPERTIES EXPORT_NAME TerminalHostPlugin)

install(TARGETS QtuiTerminalHost QtuiTerminalHostplugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/TerminalHost"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/TerminalHost"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/TerminalHost/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/TerminalHost/QtuiTerminalHost.qmltypes"
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/TerminalHost"
)
install(FILES TerminalView.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/terminalhost")
install(FILES "${PROJECT_SOURCE_DIR}/docs/third-party/libvterm-LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/Qtui/third-party"
)
