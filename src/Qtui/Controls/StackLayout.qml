import QtQml
import Qtui

Item {
    id: stackLayoutRoot

    property int currentIndex: 0
    readonly property int count: stackLayoutRoot.children.length

    onCurrentIndexChanged: updateVisibility()
    Component.onCompleted: updateVisibility()

    function updateVisibility() {
        let ch = stackLayoutRoot.children;
        for (let i = 0; i < ch.length; ++i) {
            let child = ch[i];
            let isCurrent = (i === stackLayoutRoot.currentIndex);
            child.visible = isCurrent;
            if (isCurrent) {
                child.x = 0;
                child.y = 0;
                child.width = stackLayoutRoot.width;
                child.height = stackLayoutRoot.height;
            }
        }
    }

    onWidthChanged: updateVisibility()
    onHeightChanged: updateVisibility()
}
