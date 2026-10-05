qt_add_qml_module(QtuiExtras
    URI Qtui.Extras
    SOURCES
        Sparkline.h Sparkline.cpp
        BigText.h BigText.cpp
        ToastManager.h ToastManager.cpp
        TabHeader.h TabHeader.cpp
        Gauge.h Gauge.cpp
    DEPENDENCIES TARGET QtuiCore
)

add_library(Qtui::Extras ALIAS QtuiExtras)
target_include_directories(QtuiExtras PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/extras>
)
target_link_libraries(QtuiExtras PUBLIC Qtui::Core PRIVATE Qt6::Core Qt6::Qml)
set_target_properties(QtuiExtras PROPERTIES EXPORT_NAME Extras)
set_target_properties(QtuiExtrasplugin PROPERTIES EXPORT_NAME ExtrasPlugin)
set_target_properties(QtuiExtras QtuiExtrasplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../../.."
)

install(TARGETS QtuiExtras
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QtuiExtrasplugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Extras"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Extras"
)

install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Extras/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Extras/QtuiExtras.qmltypes"
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Extras"
)
install(FILES
    Sparkline.h BigText.h ToastManager.h TabHeader.h Gauge.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/extras"
)
