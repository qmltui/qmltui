import QtQml
import Qamui

Text {
    property int type: 0
    property string enemyColor: "red"
    text: type === 0 ? "v" : (type === 1 ? "<M>" : "oXo")
    color: enemyColor
    width: type === 0 ? 1 : 3
    height: 1
}
