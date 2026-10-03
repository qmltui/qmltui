# Loader

`Loader` creates a QtUI item from a QML component when it is needed. The
created object must derive from `qtui::Item`; Qt Quick visual items are not
part of QtUI's terminal rendering tree and are rejected.

## Properties

| Property | Type | Description |
|---|---|---|
| `active` | `bool` | Creates the item when true and destroys it when false. |
| `source` | `url` | URL of a QML component to load. |
| `sourceComponent` | `Component` | Inline component to instantiate. Setting it clears `source`. |
| `item` | `Item` | The instantiated QtUI item, or `null`. |
| `status` | `Loader.Status` | `Null`, `Ready`, `Loading`, or `Error`. |
| `progress` | `real` | Source compilation progress from 0 to 1. |
| `asynchronous` | `bool` | Requests asynchronous compilation for URL sources. |
| `errorString` | `string` | Compilation or instantiation error text. |

The loader adopts the loaded item's implicit size. An explicit loader width or
height is propagated to the loaded item.

## Example

```qml
import Qtui

Loader {
    active: showDetails
    sourceComponent: Text {
        text: "Details"
    }
}
```

Access `item` only after `status` becomes `Loader.Ready`.
