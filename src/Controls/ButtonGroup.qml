import QtQml

QtObject {
    id: group
    property list<QtObject> buttons

    onButtonsChanged: {
        for (let i = 0; i < buttons.length; ++i) {
            if (buttons[i] && buttons[i].buttonGroup !== undefined) {
                buttons[i].buttonGroup = group
            }
        }
    }
}
