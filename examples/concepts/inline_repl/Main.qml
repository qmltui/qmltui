pragma ComponentBehavior: Bound
import QtQml
import Qamui

Terminal {
    id: root
    mode: Terminal.Inline
    title: "Inline REPL"

    Scrollback { id: history }

    Column {
        width: parent.width
        Text { text: "Enter a message. Escape clears the prompt, then quits." }
        TextInput {
            id: prompt
            width: parent.width
            height: 1
            focus: true
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (history.print(prompt.text)) prompt.text = ""
                    event.accepted = true
                }
            }
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (prompt.text.length > 0) prompt.text = ""
            else Qt.quit()
            event.accepted = true
        }
    }
}
