qt_add_qml_module(QtuiControls
    URI Qtui.Controls
    OUTPUT_TARGETS qtui_controls_output_targets
    SOURCES
        ForeignTypes.h
        Sparkline.h
        Sparkline.cpp
        TabBar.h
        TabBar.cpp
        Toast.h
        Toast.cpp
        ToolTip.h
        ToolTip.cpp
    QML_FILES
        AbstractButton.qml
        Button.qml
        ButtonGroup.qml
        CheckBox.qml
        Dialog.qml
        Panel.qml
        ProgressBar.qml
        RadioButton.qml
        Slider.qml
        StandardItemDelegate.qml
        BusyIndicator.qml
        StackLayout.qml
        StackView.qml
        Switch.qml
    DEPENDENCIES TARGET QtuiCore
)

target_compile_definitions(QtuiControls PRIVATE QTUI_CONTROLS_LIBRARY)
add_library(Qtui::Controls ALIAS QtuiControls)
target_include_directories(QtuiControls
    PUBLIC
        $<BUILD_INTERFACE:${Qtui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)
target_link_libraries(QtuiControls PRIVATE Qt6::Core Qt6::Qml Qtui::Core)
set_target_properties(QtuiControls PROPERTIES EXPORT_NAME Controls)
set_target_properties(QtuiControlsplugin PROPERTIES EXPORT_NAME ControlsPlugin)
set_target_properties(QtuiControls QtuiControlsplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../.."
)

install(TARGETS QtuiControls
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QtuiControlsplugin
    EXPORT QtuiTargets
    LIBRARY DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Controls"
    RUNTIME DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Controls"
)
if(qtui_controls_output_targets)   # static builds only, see Core
    install(TARGETS ${qtui_controls_output_targets}
        EXPORT QtuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Controls/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qtui/Controls/QtuiControls.qmltypes"
    ${CMAKE_CURRENT_SOURCE_DIR}/AbstractButton.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Button.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/ButtonGroup.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/CheckBox.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Dialog.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Panel.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/ProgressBar.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/RadioButton.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Slider.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/StandardItemDelegate.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/BusyIndicator.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/StackLayout.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/StackView.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Switch.qml
    DESTINATION "${QTUI_INSTALL_QMLDIR}/Qtui/Controls"
)
qtui_install_headers(Qtui/Controls FILES Sparkline.h TabBar.h Toast.h ToolTip.h)
