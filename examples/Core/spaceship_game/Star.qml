import QtQml 2.15
import Qtui

Text {
    property int speed: 1
    text: speed === 1 ? "." : (speed === 2 ? "+" : "*")
    color: speed === 1 ? "dimgray" : (speed === 2 ? "gray" : "white")
    width: 1
    height: 1
}
