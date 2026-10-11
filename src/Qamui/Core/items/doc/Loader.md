# Loader

`Loader` creates an object from a QML component when it is needed. It
instantiates any object type, not only visual ones, so a theme, controller, or
other headless `QtObject` can be loaded on demand.

## Properties

| Property | Type | Description |
|---|---|---|
| `active` | `bool` | Creates the item when true and destroys it when false. |
| `source` | `url` | URL of a QML component to load. |
| `sourceComponent` | `Component` | Inline component to instantiate. Setting it clears `source`. |
| `item` | `QtObject` | The instantiated object, or `null`. |
| `status` | `Loader.Status` | `Null`, `Ready`, `Loading`, or `Error`. |
| `progress` | `real` | Source compilation progress from 0 to 1. |
| `asynchronous` | `bool` | Requests asynchronous compilation for URL sources. |
| `errorString` | `string` | Compilation or instantiation error text. |

When the loaded object is a `qamui::Item` it becomes a visual child of the
loader: the loader adopts its implicit size, and an explicit loader width or
height is propagated to it. No sizing rule applies to a non-visual object; the
loader keeps its own zero implicit size and the object is owned by the loader
without joining the visual tree.

## Example

```qml
import Qamui

Loader {
    active: showDetails
    sourceComponent: Text {
        text: "Details"
    }
}
```

Loading a headless object works the same way:

```qml
import Qamui
import QtQml

Loader {
    id: themeLoader
    source: darkMode ? "DarkTheme.qml" : "LightTheme.qml"
}
```

```qml
// DarkTheme.qml
import QtQml

QtObject {
    readonly property color accent: "cyan"
}
```

Access `item` only after `status` becomes `Loader.Ready`.
