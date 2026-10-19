# Animation and states

QmlTui provides declarative states, property changes, and transitions tailored for
retained-mode terminal user interfaces.

## Feature Matrix

| Feature | Public Qt Only (`PRIVATE_QT=OFF`) | Private Qt (`PRIVATE_QT=ON`) |
| :--- | :--- | :--- |
| `Item.states` and `state` | Supported | Supported (shared controller) |
| `StateGroup` | Not available | Supported |
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

The unified option `QMLTUI_ENABLE_PRIVATE_QT=ON` (enabled by default in `CMakeLists.txt`) enables both `Behavior` and advanced
states/`StateGroup` support via `Qt6::QmlPrivate`. Pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` when a strict public-Qt build with zero private header dependencies is required.

## States, Transitions, and `Behavior` in QML

Use [`State`](../qml-types/state.md) and [`Transition`](../qml-types/transition.md) to coordinate multi-property changes, or attach [`Behavior`](../qml-types/behavior.md) (when `QMLTUI_ENABLE_PRIVATE_QT=ON`) to animate individual property writes automatically:

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
    border.style: Border.Rounded
    border.color: "gray"

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

## Syntax Forms

### 1. Direct Assignment (Private Qt Builds)

In builds with `QMLTUI_ENABLE_PRIVATE_QT=ON`, properties can be declared directly
on targets or qualified by item IDs:

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

- By default (`explicit: false`), expressions remain reactive: if `root.status` changes
  while the state is active, `label.text` updates immediately.
- When `explicit: true`, the expression is evaluated once on entry and frozen.
- Mixing direct assignments and map-based `properties: { ... }` in a single
  `PropertyChanges` object is rejected at compile time.

### 2. Map-Based Declarations (All Builds)

Supported across all configurations:

```qml
PropertyChanges {
    target: label
    properties: {
        "text": root.status,
        "color": "cyan"
    }
}
```

Within an `Item`, omitting `target` defaults to the containing item. Within `StateGroup`,
an explicit `target` or ID-qualified property is required.

## Binding and Restoration Lifecycle

- **Binding Suspension**: Preexisting bindings on modified properties are suspended
  rather than overwritten when entering a state.
- **Exit Restoration**: On exiting to base (or another state), if `restoreEntryValues` is
  `true` (default), the original binding or snapshot value is restored and re-evaluated.
- **Chained Transitions**: Across chained transitions (A → B → base), the original base
  state is preserved across all hops until base is reached.
- **Retained Values**: When `restoreEntryValues: false` is set on a state, modified
  properties retain their active state values upon exit, and previous entry values are
  discarded.

## Independent Groups and Atomic Overlap Rejection

Multiple state controllers (an item's `states` and one or more `StateGroup` objects) can
coexist within an application.

- Each `Item` and `StateGroup` acts as an independent property owner.
- Property aliases are canonicalized to their underlying target property before ownership
  checks.
- **Preflight Check**: Before applying any property changes or stopping animations,
  the controller verifies whether any requested property is currently owned by another
  active controller.
- If an overlap collision occurs, a warning is issued and the **entire activation is
  atomically rejected**, leaving the current state unchanged.
- Ownership is released when state exit restoration completes.

## Terminal-Specific Transition Semantics

- **Integer Cell Geometry**: Geometry animations (`NumberAnimation` on `x`, `y`, `width`,
  `height`) operate on integer character cells, preventing sub-cell fractional jitter.
- **Duplicate Write Suppression**: When an animated property endpoint equals the current
  displayed value, animation writes and allocations are suppressed.
- **Zero Duration**: Setting `duration: 0` applies the destination value synchronously
  and completes without registering clock work or busy-looping.
- **Interruption Continuity**: If a state change interrupts an in-progress transition,
  the new transition starts interpolating smoothly from the currently displayed value.
- **Binding Overrides**: For destination properties bound to expressions, the endpoint
  is evaluated at transition start, suspended during interpolation, and reattached upon
  transition completion.
- **Precedence over Behavior**: When an animated property matches an active `Transition`,
  the transition takes precedence over any `Behavior` on that property.

See the [independent states guide example](../../examples/guides/independent_states/),
the gallery examples [`States.qml`](../../examples/gallery/examples/States.qml),
[`Animation.qml`](../../examples/gallery/examples/Animation.qml), and
[`Behavior.qml`](../../examples/gallery/examples/Behavior.qml), and the
[`State`](../qml-types/state.md), [`StateGroup`](../qml-types/stategroup.md), and
[`PropertyChanges`](../qml-types/propertychanges.md) references.
