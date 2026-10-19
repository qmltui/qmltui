pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root
    mode: Terminal.Inline
    title: "Inline REPL"

    Scrollback {
        id: history
    }

    Column {
        width: parent.width
        TextInput {
            id: prompt
            width: parent.width
            height: 1
            focus: true
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (history.print({
                        text: prompt.text,
                        color: "cyan"
                    }))
                        prompt.text = "";
                    event.accepted = true;
                }
            }
        }
        Text {
            text: "Enter a message. Escape clears the prompt, then quits."
            dim: true
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (prompt.text.length > 0)
                prompt.text = "";
            else
                Qt.quit();
            event.accepted = true;
        }
    }
}
