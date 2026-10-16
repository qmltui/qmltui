# 04: Models and views

ListModel supplies named roles. The delegate declares `service` as required,
so the model supplies it directly. The attached `ListView.isCurrentItem` binding
styles the selected row. Down changes selection and updates the active row style. TableView displays a small two-dimensional model
with explicit column widths. Larger application datasets belong in C++ models.

Build with `cmake --build build --target qmltui_tutorial_04_models_and_views` and run
`./build/examples/tutorials/04_models_and_views/qmltui_tutorial_04_models_and_views`. Press Escape to quit.

## Terminal Output Preview

```text

  Services

  Gateway
  Worker
  Database


  ┌──────────────────────┬─────────────────────────┐
  │Service               │State                    │
  ├──────────────────────┼─────────────────────────┤
  │Gateway               │Ready                    │
  │Worker                │Idle                     │
  └──────────────────────┴─────────────────────────┘

```

## Source Files

### `Main.qml`

<!-- include: examples/tutorials/04_models_and_views/Main.qml -->
```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls
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

## Key Concepts in This Chapter

- **`ListModel` Roles & `required property` Delegates**: With `pragma ComponentBehavior: Bound` active, delegates declare each model role they consume as a typed `required property` (here `required property string service`). This prevents unqualified context lookups and lets `qmllint` and `qmlls` verify role bindings at compile time.
- **`ListView.isCurrentItem` Attached Property**: `ListView` attaches `ListView.isCurrentItem` to each instantiated delegate. Binding `bold: ListView.isCurrentItem` automatically updates the visual style of the active row when **Up** or **Down** changes `currentIndex`.
- **Multi-Column `TableView`**: `TableView` renders tabular data with configurable `columns` (`{ title, width }`), box-drawing borders (`showOuterBorder`, `showColumnGrid`), and viewport virtualization. For small static tables a 2D JavaScript array works directly; for large or dynamic datasets, expose a C++ `QAbstractTableModel` or `QAbstractListModel` subclass (see [Models & Virtualized Views](../topics/models.md)).

---

[← Previous: 03: Input and focus](03-input-and-focus.md) | [Tutorial index](index.md) | [Next: 05: Controls and styling →](05-controls-and-styling.md)
