import QtQml 2.15
import Tui 1.0

Text {
    text: type === "shield" ? "[S]" : (type === "fire" ? "[F]" : "[H]")
    color: type === "shield" ? "green" : (type === "fire" ? "orange" : "magenta")
    width: 3
    height: 1
}
