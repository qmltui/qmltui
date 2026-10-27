# Animation and States

QmlTui supports declarative states, property changes, transitions, and property animations on the character-cell grid.

## Public Qt vs. Private Qt Builds

By default (`QMLTUI_ENABLE_PRIVATE_QT=ON`), QmlTui automatically links `Qt6::QmlPrivate` when available to provide `Behavior` and binding-aware `PropertyChanges`. If `Qt6::QmlPrivate` is not installed, CMake emits a warning and builds the public-Qt subset; pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` when building strictly against public Qt headers.

| Feature | Public Qt Only (`PRIVATE_QT=OFF`) | Private Qt (`PRIVATE_QT=ON`) |
| :--- | :--- | :--- |
| `Item.states` and `state` | Supported | Supported (shared controller) |
| `StateGroup` | Supported | Supported |
| Map-based `PropertyChanges { properties: { ... } }` | Supported | Supported |
| Direct assignment syntax `PropertyChanges { target.prop: val }` | Not available | Supported |
| `PropertyChanges.explicit` | Ignored | Supported (`false` default reactive) |
| Value restoration | Snapshot-based | Binding-aware & snapshot-based |
| Binding preservation across states (A → B → base) | Restores value | Restores original bindings |
| `restoreEntryValues: false` | Supported | Supported |
| Independent groups & atomic collision preflight | Supported | Supported |
| Zero-duration (`duration: 0`) synchronous execution | Supported | Supported |
| Endpoint equality write suppression | Supported | Supported |
| Interrupted transition starts from displayed value | Supported | Supported |
| `Behavior on property` | Not available | Supported |

## States, Transitions, and `Behavior`

Use [`State`](../qml-types/state.md) and [`Transition`](../qml-types/transition.md) to coordinate multi-property changes, or attach [`Behavior`](../qml-types/behavior.md) (when `QMLTUI_ENABLE_PRIVATE_QT=ON`) to animate individual property writes:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Rectangle {
    id: card
    property bool expanded: false

    width: 24
    height: 3
    color: "#16202a"
    border {
        style: Border.Rounded
        color: "gray"
    }

    Behavior on width {
        NumberAnimation { duration: 150 }
    }

    states: [
        State {
            name: "expanded"
            when: card.expanded
            PropertyChanges {
                target: card
                width: 48
                border.color: "cyan"
            }
        }
    ]

    transitions: [
        Transition {
            NumberAnimation {
                property: "width"
                duration: 200
                easing.type: Easing.OutQuad
            }
            ColorAnimation {
                duration: 200
            }
        }
    ]
}
```

## `PropertyChanges` Syntax Forms

### Direct Assignment (`QMLTUI_ENABLE_PRIVATE_QT=ON`)

When built with `QMLTUI_ENABLE_PRIVATE_QT=ON`, properties can be assigned directly on a `target` or qualified by item `id`:

```qml
PropertyChanges {
    target: label
    text: root.status
    color: "cyan"
}

PropertyChanges {
    label.text: root.status
    card.border.color: "green"
}
```

By default (`explicit: false`), expressions remain reactive while the state is active: if `root.status` changes, `label.text` updates. When `explicit: true`, the expression is evaluated once on state entry. Mixing direct assignments and map-based `properties: { ... }` in a single `PropertyChanges` object is rejected at compile time.

### Map-Based Declarations (All Builds)

The map form works in both public-Qt and private-Qt builds:

```qml
PropertyChanges {
    target: label
    properties: {
        "text": root.status,
        "color": "cyan"
    }
}
```

Inside an `Item`, omitting `target` defaults to the enclosing item. Inside a `StateGroup`, an explicit `target` or `id`-qualified property is required.

## Binding and Restoration Lifecycle

When entering a state, preexisting bindings on modified properties are suspended rather than overwritten. On exiting to the base state (or transitioning through chained states `A -> B -> base`), `restoreEntryValues: true` (the default) restores the original base binding or value. Setting `restoreEntryValues: false` on a `State` leaves the modified properties at their state values upon exit.

## Independent State Controllers

An application can combine `Item.states` with one or more [`StateGroup`](../qml-types/stategroup.md) instances for orthogonal state machines. Each `Item` and `StateGroup` tracks the properties it currently owns (resolving QML property aliases to their underlying target property). Before applying a state change, the controller checks whether any target property is already owned by another active controller; if a collision is detected, a warning is logged and the activation is rejected atomically without partially mutating the scene.

## Transition Semantics on the Cell Grid

- Geometry animations (`NumberAnimation` on `x`, `y`, `width`, `height`) step through integer character cells.
- When an animated property's destination equals its current displayed value, writes are suppressed.
- Setting `duration: 0` applies the destination value synchronously without scheduling timer ticks.
- If a state change interrupts a running transition, the new transition interpolates from the currently displayed value.
- When a property is covered by both an active `Transition` and a `Behavior`, the `Transition` takes precedence.

See [`examples/recipes/independent_states/`](../../examples/recipes/independent_states/), the gallery examples [`States.qml`](../../examples/gallery/examples/States.qml), [`Animation.qml`](../../examples/gallery/examples/Animation.qml), and [`Behavior.qml`](../../examples/gallery/examples/Behavior.qml), and the [`State`](../qml-types/state.md), [`StateGroup`](../qml-types/stategroup.md), and [`PropertyChanges`](../qml-types/propertychanges.md) references.

![States recorded demo](../assets/casts/components/states.gif)

![Behavior recorded demo](../assets/casts/components/behavior.gif)
