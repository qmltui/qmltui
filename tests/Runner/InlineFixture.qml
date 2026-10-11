import QtQml
import QtQml.Models
import Qamui

// Inline-mode fixture for the scrollback assertions. History leaves through a
// model-backed Scrollback and the imperative print(); the live region is a
// title line and a one-line prompt. Function keys append rows so the prompt
// keeps focus: F1 a model row, F2 a wide-character row, F3 a styled row.
Terminal {
    id: root
    mode: Terminal.Inline
    focus: true

    ListModel { id: rows }

    Scrollback {
        id: history
        model: rows
        delegate: Text {
            required property string display
            required property string tone
            text: display
            color: tone
            bold: tone === "#ffcc00"
        }
    }

    Column {
        Text { text: "INLINE FIXTURE"; color: "#7ee0ff" }
        Row {
            Text { text: "> " }
            TextInput {
                id: prompt
                width: 40
                focus: true
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        if (prompt.text.length > 0 && history.print(prompt.text))
                            prompt.text = "";
                        event.accepted = true;
                    }
                }
            }
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_F1) {
            rows.append({ display: "MODEL ROW " + rows.count, tone: "#ffffff" });
        } else if (event.key === Qt.Key_F2) {
            rows.append({ display: "wide 世界 row", tone: "#ffffff" });
        } else if (event.key === Qt.Key_F3) {
            rows.append({ display: "STYLED ROW", tone: "#ffcc00" });
        } else if (event.key === Qt.Key_Escape
                   || (event.key === Qt.Key_Q && (event.modifiers & Qt.ControlModifier))) {
            Qt.quit();
        } else {
            return;
        }
        event.accepted = true;
    }
}
