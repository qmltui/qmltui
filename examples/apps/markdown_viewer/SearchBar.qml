import QtQml
import Qtui

Rectangle {
    id: searchBar
    height: 3
    color: theme.surface

    required property var theme
    property string queryText: ""
    property int currentMatch: 0
    property int totalMatches: 0
    property bool isSearching: false

    signal queryChanged(string query)
    signal nextMatch()
    signal previousMatch()
    signal closeSearch()

    border.style: "single"
    border.color: theme.warning

    Row {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        spacing: 2

        Text {
            text: "🔍 Search:"
            color: theme.warning
            bold: true
        }

        TextInput {
            id: input
            width: 30
            height: 1
            focus: searchBar.isSearching
            color: theme.textPrimary
            cursorColor: theme.warning
            placeholderText: "Type search phrase..."
            placeholderColor: theme.textMuted
            text: searchBar.queryText

            onTextChanged: {
                searchBar.queryText = text
                searchBar.queryChanged(text)
            }

            onAccepted: {
                searchBar.nextMatch()
            }
        }

        Text {
            text: "│"
            color: theme.border
        }

        Text {
            text: (searchBar.totalMatches > 0) ?
                  ("Match " + (searchBar.currentMatch + 1) + " of " + searchBar.totalMatches) :
                  (searchBar.queryText.length > 0 ? "No matches" : "")
            color: (searchBar.totalMatches > 0) ? theme.accent : theme.textMuted
            bold: true
        }

        Text {
            text: "│"
            color: theme.border
        }

        Text {
            text: "[Enter/n] Next  [N] Prev  [Esc] Close"
            color: theme.textMuted
        }
    }
}
