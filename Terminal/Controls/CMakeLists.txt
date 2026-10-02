qt_add_qml_module(TerminalControls
    URI Terminal.Controls
    VERSION 1.0
    QML_FILES
        Button.qml
        ButtonGroup.qml
        CheckBox.qml
        Panel.qml
        RadioButton.qml
        Slider.qml
        Spinner.qml
        Switch.qml
    DEPENDENCIES TARGET Qtui
)

target_link_libraries(TerminalControls PRIVATE Qt6::Core Qt6::Qml Qtui)
