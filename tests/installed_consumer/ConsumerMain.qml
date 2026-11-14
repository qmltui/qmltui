pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            event.accepted = true;
            Qt.quit();
        }
    }

    Button {
        objectName: "moduleBtn"
        text: "Installed Module"
    }
}
