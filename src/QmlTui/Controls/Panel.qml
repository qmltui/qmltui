pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// Bordered container that places optional `title` and `titleTrailing` items
// inline on the top border row, plus `footer` and `footerTrailing` items on
// the bottom border row, padded by one space cell on each side.
Rectangle {
    id: control

    property Item title: null
    property int titleAlignment: Qt.AlignLeft
    property Item titleTrailing: null
    property Item footer: null
    property Item footerTrailing: null

    border.style: Border.Single

    onTitleChanged: d.layout()
    onTitleTrailingChanged: d.layout()
    onFooterChanged: d.layout()
    onFooterTrailingChanged: d.layout()
    Component.onCompleted: d.layout()

    QtObject {
        id: d

        property Item prevTitle: null
        property Item prevTitleTrailing: null
        property Item prevFooter: null
        property Item prevFooterTrailing: null

        readonly property int titleWidth: control.title && control.title.visible ? (control.title.implicitWidth > 0 ? control.title.implicitWidth : control.title.width) : 0
        readonly property int titleTrailingWidth: control.titleTrailing && control.titleTrailing.visible ? (control.titleTrailing.implicitWidth > 0 ? control.titleTrailing.implicitWidth : control.titleTrailing.width) : 0
        readonly property int footerWidth: control.footer && control.footer.visible ? (control.footer.implicitWidth > 0 ? control.footer.implicitWidth : control.footer.width) : 0
        readonly property int footerTrailingWidth: control.footerTrailing && control.footerTrailing.visible ? (control.footerTrailing.implicitWidth > 0 ? control.footerTrailing.implicitWidth : control.footerTrailing.width) : 0

        readonly property bool titleActive: d.titleWidth > 0
        readonly property bool titleTrailingActive: d.titleTrailingWidth > 0
        readonly property bool footerActive: d.footerWidth > 0
        readonly property bool footerTrailingActive: d.footerTrailingWidth > 0

        readonly property int titleSpanWidth: d.titleActive ? d.titleWidth + 2 : 0
        readonly property int titleTrailingSpanWidth: d.titleTrailingActive ? d.titleTrailingWidth + 2 : 0
        readonly property int footerSpanWidth: d.footerActive ? d.footerWidth + 2 : 0
        readonly property int footerTrailingSpanWidth: d.footerTrailingActive ? d.footerTrailingWidth + 2 : 0

        readonly property int titleTrailingStartX: Math.max(1, control.width - d.titleTrailingSpanWidth - 2)
        readonly property int titleStartX: {
            if (control.titleAlignment === Qt.AlignLeft) {
                return 2;
            }
            let maxStartX = d.titleTrailingActive ? (d.titleTrailingStartX - d.titleSpanWidth) : (control.width - d.titleSpanWidth - 2);
            if (control.titleAlignment === Qt.AlignHCenter) {
                let centeredX = Math.floor((control.width - d.titleSpanWidth) / 2);
                return Math.max(1, Math.min(centeredX, maxStartX));
            } else if (control.titleAlignment === Qt.AlignRight) {
                return Math.max(1, maxStartX);
            }
            return 2;
        }
        readonly property int footerStartX: 2
        readonly property int footerTrailingStartX: Math.max(1, control.width - d.footerTrailingSpanWidth - 2)
        readonly property int bottomRowY: Math.max(0, control.height - 1)

        function syncSlot(item: Item, prevItem: Item, xPos: int, yPos: int, zOrder: real): Item {
            if (prevItem && prevItem !== item) {
                prevItem.visible = false;
                prevItem.parent = null;
            }
            if (item) {
                item.parent = control;
                item.x = xPos;
                item.y = yPos;
                item.z = zOrder;
            }
            return item;
        }

        function layout() {
            d.prevTitleTrailing = d.syncSlot(control.titleTrailing, d.prevTitleTrailing, d.titleTrailingStartX + 1, 0, 0.2);
            d.prevTitle = d.syncSlot(control.title, d.prevTitle, d.titleStartX + 1, 0, 0.4);
            d.prevFooter = d.syncSlot(control.footer, d.prevFooter, d.footerStartX + 1, d.bottomRowY, 0.2);
            d.prevFooterTrailing = d.syncSlot(control.footerTrailing, d.prevFooterTrailing, d.footerTrailingStartX + 1, d.bottomRowY, 0.4);
        }

        onTitleStartXChanged: d.layout()
        onTitleTrailingStartXChanged: d.layout()
        onFooterTrailingStartXChanged: d.layout()
        onBottomRowYChanged: d.layout()
    }

    Text {
        id: titleTrailingMask

        x: d.titleTrailingStartX
        y: 0
        z: 0.1
        visible: d.titleTrailingActive
        text: " ".repeat(d.titleTrailingSpanWidth)
    }

    Text {
        id: titleMask

        x: d.titleStartX
        y: 0
        z: 0.3
        visible: d.titleActive
        text: " ".repeat(d.titleSpanWidth)
    }

    Text {
        id: footerMask

        x: d.footerStartX
        y: d.bottomRowY
        z: 0.1
        visible: d.footerActive
        text: " ".repeat(d.footerSpanWidth)
    }

    Text {
        id: footerTrailingMask

        x: d.footerTrailingStartX
        y: d.bottomRowY
        z: 0.3
        visible: d.footerTrailingActive
        text: " ".repeat(d.footerTrailingSpanWidth)
    }
}
