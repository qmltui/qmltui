qt_add_qml_module(QmlTuiControls
    URI QmlTui.Controls
    ${QMLTUI_STATIC_QML_MODULE}
    OUTPUT_TARGETS qmltui_controls_output_targets
    SOURCES
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
        BusyIndicator.qml
        StackView.qml
        Switch.qml
    DEPENDENCIES TARGET QmlTuiCore
)

get_target_property(qmltui_controls_library_type QmlTuiControls TYPE)
if(qmltui_controls_library_type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(QmlTuiControls PUBLIC QMLTUI_CONTROLS_STATIC)
    target_link_libraries(QmlTuiControls INTERFACE QmlTuiControlsplugin)
else()
    target_compile_definitions(QmlTuiControls PRIVATE QMLTUI_CONTROLS_LIBRARY)
endif()
add_library(qmltui::controls ALIAS QmlTuiControls)
add_library(QmlTui::Controls ALIAS QmlTuiControls)
target_include_directories(QmlTuiControls
    PUBLIC
        $<BUILD_INTERFACE:${qmltui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)
target_link_libraries(QmlTuiControls PUBLIC qmltui::core)
set_target_properties(QmlTuiControls PROPERTIES EXPORT_NAME Controls)
set_target_properties(QmlTuiControlsplugin PROPERTIES EXPORT_NAME ControlsPlugin)
file(RELATIVE_PATH _qmltui_rel_libdir
    "${CMAKE_INSTALL_PREFIX}/${QMLTUI_INSTALL_QMLDIR}/QmlTui/Controls"
    "${CMAKE_INSTALL_FULL_LIBDIR}"
)
set_target_properties(QmlTuiControls QmlTuiControlsplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/${_qmltui_rel_libdir}"
)

install(TARGETS QmlTuiControls
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QmlTuiControlsplugin
    EXPORT QmlTuiTargets
    LIBRARY DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/Controls"
    RUNTIME DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/Controls"
    ARCHIVE DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/Controls"
)
if(qmltui_controls_output_targets)   # static builds only, see Core
    install(TARGETS ${qmltui_controls_output_targets}
        EXPORT QmlTuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/Controls/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/QmlTui/Controls/QmlTuiControls.qmltypes"
    ${CMAKE_CURRENT_SOURCE_DIR}/AbstractButton.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Button.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/ButtonGroup.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/CheckBox.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Dialog.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Panel.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/ProgressBar.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/RadioButton.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Slider.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/BusyIndicator.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/StackView.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Switch.qml
    DESTINATION "${QMLTUI_INSTALL_QMLDIR}/QmlTui/Controls"
)
qmltui_install_headers(QmlTui/Controls FILES Sparkline.h TabBar.h Toast.h ToolTip.h)
