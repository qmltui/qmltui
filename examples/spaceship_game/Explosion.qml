import QtQml 2.15
import Tui 1.0

Text {
    text: frame === 0 ? "*" : (frame === 1 ? "#" : (frame === 2 ? "+" : "."))
    color: frame === 0 ? "orange" : (frame === 1 ? "red" : (frame === 2 ? "yellow" : "gray"))
    width: 1
    height: 1
}
