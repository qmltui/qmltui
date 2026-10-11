import QtQml
import Qamui

// Default delegate for QAbstractItemModel rows. Renders the standard item
// roles the way QStyledItemDelegate does in Qt Widgets: decoration glyph,
// check box, foreground and background colors, text alignment, and a dimmed
// look for disabled items. Works as the delegate of ListView, GridView,
// Repeater, TableView, and TreeView. toggle() flips Qt::CheckStateRole
// through the model's setData() when the item is user-checkable.
Item {
    id: delegate

    required property var model
    property bool current: ListView.isCurrentItem
    property rgba textColor: "default"

    readonly property string display: {
        if (typeof model === "string" || typeof model === "number") return String(model)
        if (!model) return ""
        if (model.display !== undefined && model.display !== null) return String(model.display)
        if (model.text !== undefined && model.text !== null) return String(model.text)
        return ""
    }
    readonly property bool checkable: !!model && model.checkable === true
    readonly property int checkState: !!model && model.checkState !== undefined && model.checkState !== null
                                      ? Number(model.checkState) : -1
    readonly property bool itemEnabled: !(!!model && model.enabled === false)
    readonly property string foreground: !!model && typeof model.foreground === "string" ? model.foreground : ""
    readonly property string background: !!model && typeof model.background === "string" ? model.background : ""
    readonly property string decoration: !!model && typeof model.decoration === "string" ? model.decoration : ""
    readonly property string alignment: !!model && typeof model.textAlignment === "string" && model.textAlignment !== ""
                                        ? model.textAlignment : "left"
    readonly property string prefix: (decoration !== "" ? decoration + " " : "")
        + (checkState < 0 ? ""
           : checkState === Qt.Checked ? "[x] "
           : checkState === Qt.PartiallyChecked ? "[-] " : "[ ] ")

    signal toggled(int checkState)

    function toggle() {
        if (!checkable || !itemEnabled || !model) return false
        const index = model.modelIndex
        if (!index || !index.valid || !index.model) return false
        const next = checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
        if (!index.model.setData(index, next, Qt.CheckStateRole)) return false
        toggled(next)
        return true
    }

    implicitWidth: label.implicitWidth
    implicitHeight: 1
    width: parent ? parent.width : implicitWidth
    height: 1
    enabled: itemEnabled

    Rectangle {
        anchors.fill: parent
        visible: delegate.background !== "" && !delegate.current
        color: delegate.background !== "" ? delegate.background : "default"
    }

    Text {
        id: label
        anchors.fill: parent
        text: delegate.prefix + delegate.display
        color: delegate.foreground !== "" ? delegate.foreground : delegate.textColor
        dim: !delegate.itemEnabled
        bold: delegate.current
        reversed: delegate.current
        horizontalAlignment: delegate.alignment === "right" ? Qt.AlignRight
                           : delegate.alignment === "center" ? Qt.AlignHCenter : Qt.AlignLeft
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Space && delegate.toggle()) event.accepted = true
    }

    MouseArea {
        anchors.fill: parent
        onClicked: delegate.toggle()
    }
}
