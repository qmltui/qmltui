import QtQml
import Qtui

Item {
    id: root

    property int xTickCount: 3
    property int yTickCount: 3
    property string axisColor: "#9aa7bb"

    readonly property string horizontalLine: makeHorizontalLine(width, xTickCount)
    readonly property string verticalLine: makeVerticalLine(height, yTickCount)

    function makeHorizontalLine(columnCount, tickCount) {
        const count = Math.max(0, Math.floor(columnCount));
        if (count === 0)
            return "";

        let characters = [];
        for (let column = 0; column < count; ++column)
            characters.push("─");

        characters[0] = "└";
        if (count > 1)
            characters[count - 1] = "┘";

        const ticks = Math.max(0, Math.floor(tickCount));
        for (let tick = 1; tick < ticks - 1; ++tick) {
            const column = Math.round((count - 1) * tick / (ticks - 1));
            if (column > 0 && column < count - 1)
                characters[column] = "┴";
        }
        return characters.join("");
    }

    function makeVerticalLine(rowCount, tickCount) {
        const count = Math.max(0, Math.floor(rowCount));
        if (count === 0)
            return "";

        let characters = [];
        for (let row = 0; row < count; ++row)
            characters.push("│");

        characters[0] = "┌";
        if (count > 1)
            characters[count - 1] = "└";

        const ticks = Math.max(0, Math.floor(tickCount));
        for (let tick = 1; tick < ticks - 1; ++tick) {
            const row = Math.round((count - 1) * tick / (ticks - 1));
            if (row > 0 && row < count - 1)
                characters[row] = "├";
        }
        return characters.join("\n");
    }

    Text {
        anchors.top: parent.top
        anchors.left: parent.left
        width: 1
        height: parent.height
        text: root.verticalLine
        color: root.axisColor
    }

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        text: root.horizontalLine
        color: root.axisColor
    }
}
