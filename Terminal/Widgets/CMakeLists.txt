qt_add_qml_module(QtuiWidgets
    URI Qtui.Widgets
    PLUGIN_TARGET QtuiWidgetsPlugin
    SOURCES
        Sparkline.h Sparkline.cpp
        BigText.h BigText.cpp
        TableView.h TableView.cpp
        TreeView.h TreeView.cpp
        ToastManager.h ToastManager.cpp
        TabHeader.h TabHeader.cpp
        Gauge.h Gauge.cpp
        Paragraph.h Paragraph.cpp
    DEPENDENCIES TARGET QtuiCore
)

add_library(Qtui::Widgets ALIAS QtuiWidgets)
target_include_directories(QtuiWidgets PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui/widgets>
)
target_link_libraries(QtuiWidgets PUBLIC QtuiCore PRIVATE Qt6::Core Qt6::Qml util)
set_target_properties(QtuiWidgets PROPERTIES EXPORT_NAME Widgets)
set_target_properties(QtuiWidgetsPlugin PROPERTIES EXPORT_NAME WidgetsPlugin)
set_target_properties(QtuiWidgets QtuiWidgetsPlugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../.."
)

install(TARGETS QtuiWidgets QtuiWidgetsPlugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Widgets"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Widgets"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Widgets/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Widgets/QtuiWidgets.qmltypes"
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Widgets"
)
install(FILES
    Sparkline.h BigText.h TableView.h TreeView.h ToastManager.h TabHeader.h
    Gauge.h Paragraph.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/widgets"
)
