---
title: Repeater
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::Repeater
category: layout
summary: Instantiates a delegate component for each item in a data model.
---

# Repeater QML type

Instantiates a delegate component for each item in a data model. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | Holds the data model providing entries for instantiation: an integer count, an array of strings or objects, or a C++ `QAbstractItemModel`. |
| `delegate` | `Component` |  | Holds the component instantiated for each item in the model. This is the default property of Repeater. |
| `rootIndex` | `QModelIndex` |  | Holds the parent index whose direct children the repeater instantiates when using a hierarchical item model; an invalid index specifies the top level. |

## Detailed Description

Repeater is used to create multiple items from a template `delegate` based on a
`model`. Created items are automatically inserted as visual children of the
Repeater's parent item (for example, inside a [Column](column.md) or [Row](row.md)).

Each instantiated delegate receives context properties such as `index` and `modelData`,
or role names when using a list or table model. To ensure high performance and compile-time
binding resolution, prefer declaring `required property` bindings in the delegate.

Repeater instantiates all delegates at once and does not virtualize items. For large,
scrollable lists or tables, prefer [ListView](listview.md) or [TableView](tableview.md).

### Example

```qml
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Repeater {
            model: ["Build: OK", "Tests: 212 passed", "Coverage: 94%"]
            Text {
                required property string modelData
                required property int index
                text: (index + 1) + ". " + modelData
            }
        }
    }
}
```
