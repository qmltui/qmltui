pragma ComponentBehavior: Bound
import Qamui

// A footer key chip: reversed key cap followed by its action label.
Row {
    id: hint
    property string key: ""
    property string label: ""
    property bool active: false
    property string activeColor: "#00f0ff"
    spacing: 1

    Text {
        text: " " + hint.key + " "
        reversed: true
        bold: true
        color: hint.active ? hint.activeColor : Theme.subtle
    }
    Text {
        text: hint.label
        bold: hint.active
        color: hint.active ? hint.activeColor : Theme.muted
    }
}
