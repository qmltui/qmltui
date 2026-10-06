import QtQml
import Qtui

Rectangle {
    id: control
    property string title: ""
    property int titleAlignment: Qt.AlignLeft
    property rgba titleColor: Palette.windowText

    border.style: "single"
    border.color: Palette.mid

    Text {
        id: titleText
        visible: control.title !== ""
        text: " " + control.title + " "
        color: control.titleColor
        
        y: 0
        x: {
            if (control.titleAlignment === Qt.AlignLeft) {
                return 2;
            } else if (control.titleAlignment === Qt.AlignHCenter) {
                return Math.max(1, Math.floor((control.width - titleText.implicitWidth) / 2));
            } else if (control.titleAlignment === Qt.AlignRight) {
                return Math.max(1, control.width - titleText.implicitWidth - 2);
            }
            return 2;
        }
    }
}
