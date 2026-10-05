pragma ComponentBehavior: Bound

import QtQml
import Qtui
import Qtui.Canvas

Terminal {
    id: rootWindow

    fillTerminal: true
    color: "#07172f"
    useAlternateScreen: true

    property bool paused: false
    property real phase: 0.0
    property int windowStart: 0
    property var animatedCyanPoints: []
    property var animatedYellowPoints: []
    property var linePoints: []

    readonly property var barValues: [4, 5, 14, 45, 82, 100, 82, 45, 14, 5, 4]
    readonly property var heavyPayload: [
        [1965, 8200], [1967, 5400], [1981, 65400], [1989, 30800],
        [1997, 10200], [2004, 11600], [2014, 4500], [2016, 7900], [2018, 1500]
    ]
    readonly property var mediumPayload: [
        [1963, 29500], [1964, 30600], [1965, 21000], [1966, 17900],
        [1975, 17500], [1982, 8300], [1988, 18300], [1990, 9900],
        [1992, 9100], [1994, 10500], [1997, 6200], [1999, 18000],
        [2001, 10000], [2002, 8100], [2010, 2600], [2013, 13600],
        [2017, 8000]
    ]
    readonly property var smallPayload: [
        [1961, 18500], [1962, 14900], [1975, 21400], [1980, 12800],
        [1988, 11000], [1993, 7600], [1996, 10600], [2000, 13100],
        [2008, 12600], [2012, 10000], [2015, 10600], [2019, 7300]
    ]

    function clamp(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function mapX(value, minimum, maximum, pixelWidth) {
        if (pixelWidth <= 1 || maximum <= minimum)
            return 0;
        return Math.round((value - minimum) / (maximum - minimum) * (pixelWidth - 1));
    }

    function mapY(value, minimum, maximum, pixelHeight) {
        if (pixelHeight <= 1 || maximum <= minimum)
            return 0;
        return Math.round((maximum - value) / (maximum - minimum) * (pixelHeight - 1));
    }

    function makeWave(pixelWidth, pixelHeight, period, amplitude, offset) {
        if (pixelWidth <= 1 || pixelHeight <= 1)
            return [];

        const count = Math.floor(pixelWidth / 2);
        let points = new Array(count * 2);
        const middle = pixelHeight / 2;
        const usableAmplitude = pixelHeight * amplitude;
        const factor = (pixelWidth > 1) ? Math.PI * 2 * period / (pixelWidth - 1) : 0;
        const phaseOffset = rootWindow.phase + offset;
        const maxY = pixelHeight - 1;

        for (let i = 0; i < count; i++) {
            const x = i * 2;
            const angle = x * factor + phaseOffset;
            let y = Math.round(middle - Math.sin(angle) * usableAmplitude);
            if (y < 0) y = 0;
            else if (y > maxY) y = maxY;
            const idx = i * 2;
            points[idx] = x;
            points[idx + 1] = y;
        }
        return points;
    }

    function makeLine(pixelWidth, pixelHeight) {
        let points = [];
        if (pixelWidth <= 1 || pixelHeight <= 1)
            return points;

        for (let x = 0; x < pixelWidth; x += 2) {
            const value = x / (pixelWidth - 1) * 5.0;
            if (value < 1.0 || value > 4.0)
                continue;
            points.push(x, mapY(value, 0.0, 5.0, pixelHeight));
        }
        return points;
    }

    function updatePlots() {
        rootWindow.animatedCyanPoints = rootWindow.makeWave(
            animatedCyanCanvas.pixelWidth,
            animatedCyanCanvas.pixelHeight,
            1.8,
            0.42,
            0.0
        );
        rootWindow.animatedYellowPoints = rootWindow.makeWave(
            animatedYellowCanvas.pixelWidth,
            animatedYellowCanvas.pixelHeight,
            1.45,
            0.30,
            Math.PI / 2
        );
        if (rootWindow.linePoints.length === 0)
            rootWindow.linePoints = rootWindow.makeLine(lineCanvas.pixelWidth, lineCanvas.pixelHeight);
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            event.accepted = true;
            Qt.quit();
            return;
        }
        if (event.key === Qt.Key_Space) {
            rootWindow.paused = !rootWindow.paused;
            event.accepted = true;
        }
    }

    Timer {
        interval: 250
        running: rootWindow.visible && !rootWindow.paused
        repeat: true
        onTriggered: {
            rootWindow.phase += 0.32;
            rootWindow.windowStart += 1;
            rootWindow.updatePlots();
        }
    }

    Component.onCompleted: rootWindow.updatePlots()

    Chart {
        id: animatedPanel
        title: qsTr("Qtui chart demo")
        xAxisTitle: qsTr("X Axis")
        yAxisTitle: qsTr("Y Axis")
        xLabels: [
            rootWindow.windowStart.toString(),
            (rootWindow.windowStart + 10).toString(),
            (rootWindow.windowStart + 20).toString()
        ]
        yLabels: [qsTr("20"), qsTr("0"), qsTr("-20")]
        yLabelWidth: 6
        statusText: rootWindow.paused ? qsTr("PAUSED") : qsTr("RUNNING")
        statusColor: rootWindow.paused ? "#ffcc33" : "#65d889"

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 1
        anchors.leftMargin: 1
        width: Math.floor((rootWindow.width - 3) * 0.61)
        height: Math.floor((rootWindow.height - 3) / 2)

        BrailleLineSeries {
            id: animatedCyanCanvas
            anchors.fill: parent
            color: "#26b6dc"
            points: rootWindow.animatedCyanPoints
            onWidthChanged: rootWindow.updatePlots()
            onHeightChanged: rootWindow.updatePlots()
        }

        BrailleLineSeries {
            id: animatedYellowCanvas
            anchors.fill: parent
            color: "#f1cf24"
            points: rootWindow.animatedYellowPoints
        }
    }

    Chart {
        id: barPanel
        title: qsTr("Bar chart")
        xLabels: [qsTr("0"), qsTr("50"), qsTr("100")]
        yLabels: [qsTr("100"), qsTr("50"), qsTr("0")]
        yLabelWidth: 6

        anchors.top: parent.top
        anchors.left: animatedPanel.right
        anchors.right: parent.right
        anchors.topMargin: 1
        anchors.leftMargin: 1
        anchors.rightMargin: 1
        height: animatedPanel.height

        Canvas {
            id: barCanvas
            anchors.fill: parent
            renderMode: Canvas.HalfBlock

            Repeater {
                model: rootWindow.barValues.length
                delegate: CanvasRectangle {
                    required property int index
                    readonly property int barWidth: Math.max(1, Math.floor(barCanvas.pixelWidth / 22))
                    readonly property int barHeight: Math.max(
                        1,
                        Math.floor(rootWindow.barValues[index] / 100 * (barCanvas.pixelHeight - 2))
                    )

                    x: 1 + Math.floor(index * (barCanvas.pixelWidth - 2) / rootWindow.barValues.length)
                    y: barCanvas.pixelHeight - barHeight - 1
                    width: barWidth
                    height: barHeight
                    fill: true
                    color: "#2688e5"
                }
            }
        }
    }

    Chart {
        id: linePanel
        title: qsTr("Line chart")
        xAxisTitle: qsTr("X Axis")
        yAxisTitle: qsTr("Y Axis")
        xLabels: [qsTr("0"), qsTr("2.5"), qsTr("5.0")]
        yLabels: [qsTr("5.0"), qsTr("2.5"), qsTr("0")]
        yLabelWidth: 6

        anchors.top: animatedPanel.bottom
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.topMargin: 1
        anchors.leftMargin: 1
        anchors.bottomMargin: 1
        width: Math.floor((rootWindow.width - 3) / 2)

        BrailleLineSeries {
            id: lineCanvas
            anchors.fill: parent
            color: "#f1cf24"
            points: rootWindow.linePoints
            onWidthChanged: rootWindow.linePoints = rootWindow.makeLine(lineCanvas.pixelWidth, lineCanvas.pixelHeight)
            onHeightChanged: rootWindow.linePoints = rootWindow.makeLine(lineCanvas.pixelWidth, lineCanvas.pixelHeight)
        }
    }

    Chart {
        id: scatterPanel
        title: qsTr("Scatter chart")
        xAxisTitle: qsTr("Year")
        yAxisTitle: qsTr("Cost")
        xLabels: [qsTr("1960"), qsTr("1990"), qsTr("2020")]
        yLabels: [qsTr("75 000"), qsTr("37 500"), qsTr("0")]
        yLabelWidth: 8

        anchors.top: animatedPanel.bottom
        anchors.left: linePanel.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 1
        anchors.leftMargin: 1
        anchors.rightMargin: 1
        anchors.bottomMargin: 1

        Canvas {
            id: heavyCanvas
            anchors.fill: parent
            color: "#26b6dc"

            Repeater {
                model: rootWindow.heavyPayload.length
                delegate: Point {
                    required property int index
                    x: rootWindow.mapX(rootWindow.heavyPayload[index][0], 1960, 2020, heavyCanvas.pixelWidth)
                    y: rootWindow.mapY(rootWindow.heavyPayload[index][1], 0, 75000, heavyCanvas.pixelHeight)
                }
            }
        }

        Canvas {
            id: mediumCanvas
            anchors.fill: parent
            color: "#d34cc8"

            Repeater {
                model: rootWindow.mediumPayload.length
                delegate: Point {
                    required property int index
                    x: rootWindow.mapX(rootWindow.mediumPayload[index][0], 1960, 2020, mediumCanvas.pixelWidth)
                    y: rootWindow.mapY(rootWindow.mediumPayload[index][1], 0, 75000, mediumCanvas.pixelHeight)
                }
            }
        }

        Canvas {
            id: smallCanvas
            anchors.fill: parent
            color: "#f1cf24"

            Repeater {
                model: rootWindow.smallPayload.length
                delegate: Point {
                    required property int index
                    x: rootWindow.mapX(rootWindow.smallPayload[index][0], 1960, 2020, smallCanvas.pixelWidth)
                    y: rootWindow.mapY(rootWindow.smallPayload[index][1], 0, 75000, smallCanvas.pixelHeight)
                }
            }
        }
    }
}
