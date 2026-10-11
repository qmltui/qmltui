import QtQml
import Qamui

Text {
    property string type: "heal"
    text: type === "shield" ? "[S]" : (type === "fire" ? "[F]" : "[H]")
    color: type === "shield" ? "green" : (type === "fire" ? "orange" : "magenta")
    width: 3
    height: 1
}
