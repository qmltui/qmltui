# 04: Models and views

ListModel supplies named roles. The delegate declares `service` as required,
so the model supplies it directly. The attached `ListView.isCurrentItem` binding
styles the selected row. Down changes selection; the scenario checks the bold
cell in the committed frame. TableView displays a small two-dimensional model
with explicit column widths. Larger application datasets belong in C++ models.

Build with `cmake --build build --target qamui_tutorial_04_models_and_views` and run
`./build/examples/tutorials/04_models_and_views/qamui_tutorial_04_models_and_views`. Press Escape to quit.

<!-- include: examples/tutorials/04_models_and_views/Main.qml -->
```qml
pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls
import QtQml.Models

Terminal {
    id: root
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
    ListModel {
        id: services
        ListElement { service: "Gateway" }
        ListElement { service: "Worker" }
        ListElement { service: "Database" }
    }
    Column {
        x: 2; y: 1; spacing: 1
        Text { text: "Services" }
        ListView {
            id: list
            width: 30; height: 4
            focus: true
            currentIndex: 0
            model: services
            delegate: Text {
                required property string service
                text: service
                bold: ListView.isCurrentItem
            }
        }
        TableView {
            width: 50; height: 6
            showOuterBorder: true
            showColumnGrid: true
            columns: [{ title: "Service", width: 22 }, { title: "State", width: 20 }]
            model: [["Gateway", "Ready"], ["Worker", "Idle"]]
        }
    }
}
```

<!-- include: examples/tutorials/04_models_and_views/test_tutorial.js -->
```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Gateway");
    terminal.press("Down");
    terminal.expectStyle(2, 4, { character: "W", bold: true });
    terminal.captureStyled("selection.txt");
    terminal.captureText("tutorial.txt");
}
```

[Tutorial index](index.md)
