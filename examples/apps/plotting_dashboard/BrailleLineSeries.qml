import QtQml
import Qtui.Canvas

Canvas {
    id: root

    property alias points: polyline.points

    renderMode: Canvas.Braille

    BraillePolyline {
        id: polyline
    }
}
