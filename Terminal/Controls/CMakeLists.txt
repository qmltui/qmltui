qt_add_qml_module(QtuiControls
    URI Qtui.Controls
    PLUGIN_TARGET QtuiControlsPlugin
    SOURCES
        ToolTip.h
        ToolTip.cpp
    QML_FILES
        Button.qml
        ButtonGroup.qml
        CheckBox.qml
        Dialog.qml
        ConfirmationDialog.qml
        Panel.qml
        RadioButton.qml
        Slider.qml
        Spinner.qml
        Switch.qml
    DEPENDENCIES TARGET QtuiCore
)

add_library(Qtui::Controls ALIAS QtuiControls)
target_link_libraries(QtuiControls PRIVATE Qt6::Core Qt6::Qml QtuiCore)
set_target_properties(QtuiControls PROPERTIES EXPORT_NAME Controls)
set_target_properties(QtuiControlsPlugin PROPERTIES EXPORT_NAME ControlsPlugin)
set_target_properties(QtuiControls QtuiControlsPlugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../.."
)

install(TARGETS QtuiControls QtuiControlsPlugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Controls"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Controls"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Controls/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Controls/QtuiControls.qmltypes"
    ${CMAKE_CURRENT_SOURCE_DIR}/Button.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/ButtonGroup.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/CheckBox.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Dialog.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfirmationDialog.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Panel.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/RadioButton.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Slider.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Spinner.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Switch.qml
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Controls"
)
