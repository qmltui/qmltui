qt_add_qml_module(QamuiControls
    URI Qamui.Controls
    OUTPUT_TARGETS qamui_controls_output_targets
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
    DEPENDENCIES TARGET QamuiCore
)

target_compile_definitions(QamuiControls PRIVATE QAMUI_CONTROLS_LIBRARY)
add_library(Qamui::Controls ALIAS QamuiControls)
target_include_directories(QamuiControls
    PUBLIC
        $<BUILD_INTERFACE:${Qamui_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}   # generated QML type registrations
)
target_link_libraries(QamuiControls PRIVATE Qt6::Core Qt6::Qml Qamui::Core)
set_target_properties(QamuiControls PROPERTIES EXPORT_NAME Controls)
set_target_properties(QamuiControlsplugin PROPERTIES EXPORT_NAME ControlsPlugin)
set_target_properties(QamuiControls QamuiControlsplugin PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../../../.."
)

install(TARGETS QamuiControls
    EXPORT QamuiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS QamuiControlsplugin
    EXPORT QamuiTargets
    LIBRARY DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/Controls"
    RUNTIME DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/Controls"
)
if(qamui_controls_output_targets)   # static builds only, see Core
    install(TARGETS ${qamui_controls_output_targets}
        EXPORT QamuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        OBJECTS DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

install(FILES
    "${QT_QML_OUTPUT_DIRECTORY}/Qamui/Controls/qmldir"
    "${QT_QML_OUTPUT_DIRECTORY}/Qamui/Controls/QamuiControls.qmltypes"
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
    DESTINATION "${QAMUI_INSTALL_QMLDIR}/Qamui/Controls"
)
qamui_install_headers(Qamui/Controls FILES Sparkline.h TabBar.h Toast.h ToolTip.h)
