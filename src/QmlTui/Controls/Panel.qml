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
    onWidthChanged: d.layout()
    Component.onCompleted: d.layout()

    QtObject {
        id: d

        property Item prevTitle: null
        property Item prevTitleTrailing: null
        property Item prevFooter: null
        property Item prevFooterTrailing: null

        readonly property int maxSlotWidth: Math.max(0, control.width - 4)

        readonly property int rawTitleWidth: control.title && control.title.visible ? (control.title.implicitWidth > 0 ? control.title.implicitWidth : control.title.width) : 0
        readonly property int rawTitleTrailingWidth: control.titleTrailing && control.titleTrailing.visible ? (control.titleTrailing.implicitWidth > 0 ? control.titleTrailing.implicitWidth : control.titleTrailing.width) : 0
        readonly property int rawFooterWidth: control.footer && control.footer.visible ? (control.footer.implicitWidth > 0 ? control.footer.implicitWidth : control.footer.width) : 0
        readonly property int rawFooterTrailingWidth: control.footerTrailing && control.footerTrailing.visible ? (control.footerTrailing.implicitWidth > 0 ? control.footerTrailing.implicitWidth : control.footerTrailing.width) : 0

        readonly property int titleTrailingWidth: Math.min(d.rawTitleTrailingWidth, d.maxSlotWidth)
        readonly property int footerTrailingWidth: Math.min(d.rawFooterTrailingWidth, d.maxSlotWidth)

        readonly property bool titleTrailingActive: d.titleTrailingWidth > 0
        readonly property bool footerTrailingActive: d.footerTrailingWidth > 0

        readonly property int titleTrailingSpanWidth: d.titleTrailingActive ? Math.min(Math.max(0, control.width - 2), d.titleTrailingWidth + 2) : 0
        readonly property int footerTrailingSpanWidth: d.footerTrailingActive ? Math.min(Math.max(0, control.width - 2), d.footerTrailingWidth + 2) : 0

        readonly property int titleTrailingStartX: Math.max(1, control.width - d.titleTrailingSpanWidth - 2)
        readonly property int footerTrailingStartX: Math.max(1, control.width - d.footerTrailingSpanWidth - 2)

        readonly property int availTitleSpan: Math.max(0, (d.titleTrailingActive ? d.titleTrailingStartX : (control.width - 1)) - 2)
        readonly property int titleWidth: Math.min(d.rawTitleWidth, Math.max(0, d.availTitleSpan - 2), d.maxSlotWidth)
        readonly property bool titleActive: d.titleWidth > 0
        readonly property int titleSpanWidth: d.titleActive ? d.titleWidth + 2 : 0

        readonly property int titleStartX: {
            if (control.titleAlignment === Qt.AlignLeft) {
                return Math.min(2, Math.max(1, (d.titleTrailingActive ? d.titleTrailingStartX : (control.width - 1)) - d.titleSpanWidth));
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

        readonly property int availFooterSpan: Math.max(0, (d.footerTrailingActive ? d.footerTrailingStartX : (control.width - 1)) - 2)
        readonly property int footerWidth: Math.min(d.rawFooterWidth, Math.max(0, d.availFooterSpan - 2), d.maxSlotWidth)
        readonly property bool footerActive: d.footerWidth > 0
        readonly property int footerSpanWidth: d.footerActive ? d.footerWidth + 2 : 0
        readonly property int footerStartX: 2
        readonly property int bottomRowY: Math.max(0, control.height - 1)

        function syncSlot(item: Item, prevItem: Item, xPos: int, yPos: int, slotW: int, active: bool, zOrder: real): Item {
            if (prevItem && prevItem !== item) {
                prevItem.visible = false;
                prevItem.parent = null;
            }
            if (item) {
                item.parent = control;
                item.x = xPos;
                item.y = yPos;
                item.width = slotW;
                item.height = Math.max(1, item.implicitHeight > 0 ? item.implicitHeight : 1);
                item.clip = true;
                item.z = zOrder;
            }
            return item;
        }

        function layout() {
            d.prevTitleTrailing = d.syncSlot(control.titleTrailing, d.prevTitleTrailing, d.titleTrailingStartX + 1, 0, d.titleTrailingWidth, d.titleTrailingActive, 0.2);
            d.prevTitle = d.syncSlot(control.title, d.prevTitle, d.titleStartX + 1, 0, d.titleWidth, d.titleActive, 0.4);
            d.prevFooter = d.syncSlot(control.footer, d.prevFooter, d.footerStartX + 1, d.bottomRowY, d.footerWidth, d.footerActive, 0.2);
            d.prevFooterTrailing = d.syncSlot(control.footerTrailing, d.prevFooterTrailing, d.footerTrailingStartX + 1, d.bottomRowY, d.footerTrailingWidth, d.footerTrailingActive, 0.4);
        }

        onTitleStartXChanged: d.layout()
        onTitleWidthChanged: d.layout()
        onTitleTrailingStartXChanged: d.layout()
        onTitleTrailingWidthChanged: d.layout()
        onFooterWidthChanged: d.layout()
        onFooterTrailingStartXChanged: d.layout()
        onFooterTrailingWidthChanged: d.layout()
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
