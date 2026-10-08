import QtQml
import Qtui

// Shows one child at a time, sized to fill the layout. count and the layout
// follow the children list, so pages created after completion take part.
Item {
    id: stackLayoutRoot

    property int currentIndex: 0
    readonly property int count: stackLayoutRoot.children.length

    onCurrentIndexChanged: updateVisibility()
    onChildrenChanged: updateVisibility()
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
