import QtQml
import Qamui

Rectangle {
    id: control

    property string status: "info"
    property string text: ""
    property string icon: ""
    property bool showIcon: true
    property string variant: "subtle" // "subtle", "filled", "outlined", "plain"

    property string successColor: "green"
    property string warningColor: "yellow"
    property string errorColor: "red"
    property string infoColor: "cyan"
    property string badgeColor: ""
    property string badgeBackgroundColor: ""
    property bool bold: true
    property int horizontalPadding: 1

    readonly property string resolvedStatus: status.toLowerCase()

    readonly property string activeColor: {
        if (badgeColor !== "") return badgeColor
        if (resolvedStatus === "success") return successColor
        if (resolvedStatus === "warning") return warningColor
        if (resolvedStatus === "error") return errorColor
        if (resolvedStatus === "info") return infoColor
        return "white"
    }

    readonly property string resolvedIcon: {
        if (icon !== "") return icon
        if (resolvedStatus === "success") return "✓"
        if (resolvedStatus === "warning") return "⚠"
        if (resolvedStatus === "error") return "✖"
        if (resolvedStatus === "info") return "ℹ"
        return "●"
    }

    readonly property string displayText: text !== "" ? text : status.toUpperCase()

    color: {
        if (variant === "filled") {
            return badgeBackgroundColor !== "" ? badgeBackgroundColor : activeColor
        }
        return badgeBackgroundColor !== "" ? badgeBackgroundColor : "transparent"
    }

    border.style: variant === "outlined" ? "single" : "none"
    border.color: variant === "outlined" ? activeColor : "transparent"

    implicitWidth: {
        if (variant === "outlined") {
            return badgeRow.implicitWidth + (horizontalPadding * 2) + 2
        } else if (variant === "filled") {
            return badgeRow.implicitWidth + (horizontalPadding * 2)
        } else {
            return badgeRow.implicitWidth
        }
    }
    implicitHeight: variant === "outlined" ? 3 : 1

    width: implicitWidth
    height: implicitHeight

    Row {
        id: badgeRow
        anchors.centerIn: parent
        spacing: 1

        Text {
            visible: control.variant === "subtle"
            text: "["
            color: control.activeColor
            bold: control.bold
        }

        Text {
            id: iconText
            visible: control.showIcon && control.resolvedIcon !== ""
            text: control.resolvedIcon
            color: control.variant === "filled" ? "black" : control.activeColor
            bold: control.bold
        }

        Text {
            id: labelText
            text: control.displayText
            color: control.variant === "filled" ? "black" : control.activeColor
            bold: control.bold
        }

        Text {
            visible: control.variant === "subtle"
            text: "]"
            color: control.activeColor
            bold: control.bold
        }
    }
}
