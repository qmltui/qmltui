import QtQml
import QmlTui

Terminal {
    id: root
    focus: true
    color: "#101820"

    property string fixtureStatus: "READY"
    property bool hoverProbe: false
    property string mouseLog: "NONE"
    property int dragTicks: 0
    property int tickCount: 0
    property bool ticking: false

    Connections {
        target: Clipboard
        function onCopyRequested(text: string): void {
            root.fixtureStatus = "COPY REQUESTED: " + text;
        }
    }

    function requestClipboard(): void {
        Clipboard.requestText().then(function (text) {
            root.fixtureStatus = "CLIPBOARD: " + text;
        }, function (error) {
            root.fixtureStatus = "CLIPBOARD: " + error.code;
        });
    }

    // Virtual-clock probes: a repeating QML Timer counter and a QmlTui
    // animation, both armed on demand so real-clock scenarios stay still.
    Timer {
        interval: 100
        repeat: true
        running: root.ticking
        onTriggered: root.tickCount++
    }

    Rectangle {
        id: bar
        objectName: "bar"
        x: 40
        y: 20
        width: 4
        height: 1
        color: "#e06c75"
    }

    NumberAnimation {
        id: grow
        target: bar
        property: "width"
        from: 4
        to: 24
        duration: 400
    }

    Timer {
        interval: 75
        running: true
        repeat: false
        onTriggered: root.fixtureStatus = "ASYNC READY"
    }

    Text {
        x: 1
        y: 1
        text: "TESTRUNNER FIXTURE"
        color: "#ffffff"
    }

    Text {
        objectName: "statusText"
        x: 1
        y: 3
        text: "STATUS: " + root.fixtureStatus
        color: "#7fdbca"
    }

    TextInput {
        id: input
        x: 1
        y: 5
        width: 50
        placeholderText: "type text here"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 7
        text: "INPUT: " + input.text
        color: "#f1fa8c"
    }

    Text {
        x: 1
        y: 9
        text: "界Z"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 11
        text: "DUPLICATE"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 12
        text: "DUPLICATE"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 13
        text: "SIZE: " + root.width + "x" + root.height
        color: "#ffffff"
    }

    // Anchored to the right edge, so a resize() must move it.
    Text {
        x: root.width - 5
        y: 1
        text: "EDGE"
        color: "#ffffff"
    }

    // Mouse probe: press, release, hover entry, and held-motion ticks land in
    // the MOUSE line. hoverEnabled starts off so scenarios can verify that a
    // hover is dropped exactly like a terminal that never sends motion.
    MouseArea {
        id: probeArea
        objectName: "probeArea"
        x: 60
        y: 15
        width: 12
        height: 3
        hoverEnabled: root.hoverProbe
        onPressed: event => root.mouseLog = "DOWN " + mouseX + "," + mouseY
        onReleased: event => root.mouseLog = "UP " + mouseX + "," + mouseY
        onEntered: root.mouseLog = "ENTER"
        onMouseXChanged: if (pressed)
            root.dragTicks = root.dragTicks + 1
    }

    Text {
        x: 1
        y: 15
        text: "MOUSE: " + root.mouseLog
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 17
        text: "DRAG TICKS: " + root.dragTicks
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 19
        text: "TICKS: " + root.tickCount
        color: "#ffffff"
    }

    // Straddles the right edge on purpose: itemRect() must clip it.
    Text {
        objectName: "clippedMarker"
        x: root.width - 2
        y: 21
        text: "CLIP"
        color: "#ffffff"
    }

    Text {
        objectName: "hiddenMarker"
        visible: false
        x: 1
        y: 22
        text: "HIDDEN"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 23
        text: "."
        color: "#ffffff"
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_K) {
            root.fixtureStatus = "KEY K";
            event.accepted = true;
        }
        if (event.key === Qt.Key_H) {
            root.hoverProbe = true;
            root.fixtureStatus = "HOVER ARMED";
            event.accepted = true;
        }
        if (event.key === Qt.Key_T) {
            root.ticking = !root.ticking;
            event.accepted = true;
        }
        if (event.key === Qt.Key_A) {
            grow.start();
            event.accepted = true;
        }
        // Reached through typeText(): scripted typing must carry the same key
        // codes as real typing, including a digit and a symbol.
        if (event.key === Qt.Key_2 && event.text === "2") {
            root.fixtureStatus = "KEY 2";
            event.accepted = true;
        }
        if (event.key === Qt.Key_Plus && event.text === "+") {
            root.fixtureStatus = "KEY PLUS";
            event.accepted = true;
        }
    }
}
