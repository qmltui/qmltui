pragma ComponentBehavior: Bound

import QtQml
import Qtui

Rectangle {
    id: root

    property string title: ""
    property string xAxisTitle: ""
    property string yAxisTitle: ""
    property var xLabels: []
    property var yLabels: []
    property string accentColor: "#58d6f0"
    property string axisColor: "#9aa7bb"
    property string labelColor: "#d4dbea"
    property string statusText: ""
    property string statusColor: accentColor
    property int yLabelWidth: 0
    property int xTickCount: xLabels.length > 1 ? xLabels.length : 3
    property int yTickCount: yLabels.length > 1 ? yLabels.length : 3

    default property alias seriesData: plotData.data
    property alias plotArea: plotData

    readonly property int effectiveYLabelWidth: yLabelWidth > 0
        ? yLabelWidth
        : Math.max(2, maximumLabelWidth(yLabels) + 1)

    color: "#07172f"
    border.style: "single"
    border.color: "#9aa7bb"

    function maximumLabelWidth(labels) {
        let result = 0;
        for (let index = 0; index < labels.length; ++index)
            result = Math.max(result, labels[index].toString().length);
        return result;
    }

    Item {
        id: plotData

        x: 1 + root.effectiveYLabelWidth
        y: 3
        width: Math.max(0, root.width - x - 2)
        height: Math.max(0, root.height - 7)
    }

    ChartAxes {
        anchors.fill: plotData
        xTickCount: root.xTickCount
        yTickCount: root.yTickCount
        axisColor: root.axisColor
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        text: root.title
        color: root.accentColor
        bold: true
    }

    Text {
        x: plotData.x
        y: 1
        text: root.yAxisTitle
        color: root.labelColor
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 2
        y: 1
        text: root.statusText
        color: root.statusColor
        bold: true
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 2
        y: plotData.y + plotData.height - 1
        text: root.xAxisTitle
        color: root.labelColor
    }

    Repeater {
        model: root.yLabels ? root.yLabels.length : 0

        delegate: Text {
            required property int index

            readonly property int labelCount: root.yLabels.length

            x: 1
            y: plotData.y + (labelCount > 1
                ? Math.round((plotData.height - 1) * index / (labelCount - 1))
                : 0)
            width: Math.max(0, root.effectiveYLabelWidth - 1)
            height: 1
            horizontalAlignment: Qt.AlignRight
            text: root.yLabels[index] ? root.yLabels[index].toString() : ""
            color: root.labelColor
        }
    }

    Repeater {
        model: root.xLabels ? root.xLabels.length : 0

        delegate: Text {
            required property int index

            readonly property int labelCount: root.xLabels.length
            readonly property int tickX: plotData.x + (labelCount > 1
                ? Math.round((plotData.width - 1) * index / (labelCount - 1))
                : 0)

            x: index === 0
                ? plotData.x
                : (index === labelCount - 1
                    ? plotData.x + plotData.width - implicitWidth
                    : tickX - Math.floor(implicitWidth / 2))
            y: root.height - 3
            height: 1
            text: root.xLabels[index] ? root.xLabels[index].toString() : ""
            color: root.labelColor
        }
    }
}
