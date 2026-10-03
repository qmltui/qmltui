import QtQml
import Qtui

Text {
    property int frame: 0
    text: frame === 0 ? "*" : (frame === 1 ? "#" : (frame === 2 ? "+" : "."))
    color: frame === 0 ? "orange" : (frame === 1 ? "red" : (frame === 2 ? "yellow" : "gray"))
    width: 1
    height: 1
}
