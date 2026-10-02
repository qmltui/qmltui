import QtQml 2.15
import Terminal 1.0

Rectangle {
    id: control
    property string title: ""
    property int titleAlignment: Qt.AlignLeft
    property string titleColor: "white"

    color: "transparent"
    border.style: "single"
    border.color: "white"

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
                return Math.max(1, Math.floor((control.width - titleText.text.length) / 2));
            } else if (control.titleAlignment === Qt.AlignRight) {
                return Math.max(1, control.width - titleText.text.length - 2);
            }
            return 2;
        }
    }
}
