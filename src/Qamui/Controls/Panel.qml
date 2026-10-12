import QtQml
import Qamui

Rectangle {
    id: control
    property string title: ""
    property int titleAlignment: Qt.AlignLeft
    property rgba titleColor: "default"

    property string titleBadge: ""
    property rgba titleBadgeColor: "default"

    property string titleTrailing: ""
    property rgba titleTrailingColor: "default"

    property string footer: ""
    property rgba footerColor: "default"

    property string footerTrailing: ""
    property rgba footerTrailingColor: "default"

    border.style: Border.Single
    border.color: "default"

    QtObject {
        id: d
        readonly property int totalTitleWidth: (titleText.visible ? titleText.implicitWidth : 0) + (titleBadgeText.visible ? titleBadgeText.implicitWidth : 0)

        readonly property int titleStartX: {
            if (control.titleAlignment === Qt.AlignLeft) {
                return 2;
            }
            let maxStartX = titleTrailingText.visible
                ? (titleTrailingText.x - d.totalTitleWidth)
                : (control.width - d.totalTitleWidth - 2);
            if (control.titleAlignment === Qt.AlignHCenter) {
                let centeredX = Math.floor((control.width - d.totalTitleWidth) / 2);
                return Math.max(1, Math.min(centeredX, maxStartX));
            } else if (control.titleAlignment === Qt.AlignRight) {
                return Math.max(1, maxStartX);
            }
            return 2;
        }
    }

    Text {
        id: titleTrailingText
        visible: control.titleTrailing !== ""
        text: " " + control.titleTrailing + " "
        color: control.titleTrailingColor

        y: 0
        x: Math.max(1, control.width - implicitWidth - 2)
    }

    Text {
        id: titleText
        visible: control.title !== ""
        text: " " + control.title + " "
        color: control.titleColor

        y: 0
        x: d.titleStartX
    }

    Text {
        id: titleBadgeText
        visible: control.titleBadge !== ""
        text: (control.title !== "" ? "" : " ") + control.titleBadge + " "
        color: control.titleBadgeColor

        y: 0
        x: titleText.visible ? (titleText.x + titleText.implicitWidth) : d.titleStartX
    }

    Text {
        id: footerText
        visible: control.footer !== ""
        text: " " + control.footer + " "
        color: control.footerColor

        y: Math.max(0, control.height - 1)
        x: 2
    }

    Text {
        id: footerTrailingText
        visible: control.footerTrailing !== ""
        text: " " + control.footerTrailing + " "
        color: control.footerTrailingColor

        y: Math.max(0, control.height - 1)
        x: Math.max(1, control.width - implicitWidth - 2)
    }
}
