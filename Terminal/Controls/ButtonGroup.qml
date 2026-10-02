import QtQml 2.15

QtObject {
    id: group
    property list<QtObject> buttons

    onButtonsChanged: {
        for (var i = 0; i < buttons.length; ++i) {
            if (buttons[i] && buttons[i].buttonGroup !== undefined) {
                buttons[i].buttonGroup = group
            }
        }
    }
}
