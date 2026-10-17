import QtQml

// Non-visual group that links mutually exclusive checkable controls (such as
// RadioButton) by assigning itself to each member's `buttonGroup` property.
QtObject {
    id: group
    property list<QtObject> buttons

    onButtonsChanged: {
        for (let i = 0; i < buttons.length; ++i) {
            if (buttons[i] && buttons[i].buttonGroup !== undefined) {
                buttons[i].buttonGroup = group;
            }
        }
    }
}
