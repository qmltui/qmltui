import QtQml 2.15
import Terminal 1.0

Text {
    text: type === 0 ? "v" : (type === 1 ? "<M>" : "oXo")
    color: enemyColor
    width: type === 0 ? 1 : 3
    height: 1
}
