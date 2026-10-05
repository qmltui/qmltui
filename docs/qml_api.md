# QML API reference

Qtui 0.2 provides a terminal-rendered, Qt Quick-style QML API. Its core
model/view/delegate names and delegate contracts are source-compatible with the
documented Qt Quick subset below. Qtui remains a terminal toolkit: geometry is
cell-based, delegates must derive from `Qtui.Item`, and APIs tied to the Qt Quick
scene graph are intentionally not implemented.

## Model and delegate compatibility

All views accept ready inline components and components that become ready
asynchronously. Delegate creation preserves the component's lexical QML context,
including `pragma ComponentBehavior: Bound`. A delegate root that is not a
`Qtui.Item`, or a component that enters the error state, is rejected with a QML
warning instead of being partially installed.

For map and `QAbstractItemModel` data, named roles are available both as required
delegate properties and through `model`. `modelData` contains the row or cell
value described in the table below. `dataChanged` updates only the roles named by
the model when the model supplies such a list.

| Type | Standard delegate properties | `modelData` | Qtui-specific behavior |
| --- | --- | --- | --- |
| `ListView` | `index`, `model`, named roles; attached `ListView.view` and `ListView.isCurrentItem` | Row value or role map | Cell-based scrolling and highlighting |
| `GridView` | `index`, `model`, named roles | Row value or role map | Fixed `cellWidth` and `cellHeight` |
| `Repeater` | `index`, `model`, named roles | Row value or role map | Inserts delegates into its visual parent |
| `TableView.delegate` | `row`, `column`, `index`, `current`, `selected`, `model`, named roles | Cell value | One delegate instance per cell |
| `TableView.rowDelegate` | `row`, `index`, `model`, named roles | Entire row | Deprecated 0.1 row-scoped compatibility path |
| `TreeView` | `treeView`, `isTreeNode`, `row`, `column`, `current`, `selected`, `depth`, `expanded`, `hasChildren`, `model`, named roles | Node role map | Column 0 is the tree column |
| `StreamView` | `index`, `model`, named roles | Row role map | Appends rendered rows; existing output is not rewritten |

`currentIndex`, `selectedRow`, `currentRow`, and `currentColumn` use `-1` for no
current item where applicable. Model destruction clears the view safely. Model
replacement disconnects the old model before a new generation is observed.

## ListView

Properties include `model`, `delegate`, `orientation`, `spacing`, `currentIndex`,
`highlightColor`, and `highlightBgColor`. `currentIndex` defaults to `-1` and can
be reset to `-1`.

Use the attached API for selection state:

```qml
ListView {
    model: applicationModel
    delegate: Text {
        required property string display
        color: ListView.isCurrentItem ? "black" : "white"
    }
}
```

See the [ListView example](../examples/components/list_view/Main.qml).

## TableView

`delegate` follows Qt Quick's cell-scoped shape. `row`, `column`, `current`, and
`selected` identify the represented cell. Qtui's `columns` descriptors continue
to provide terminal column titles, keys, widths, and alignment.

`rowDelegate` retains the Qtui 0.1 row-scoped delegate contract for migration and
is planned for removal in 0.3. When both properties are set, `delegate` wins and
Qtui emits a warning.

Selection properties are `currentRow` (read-only alias of `selectedRow`),
`selectedRow`, and `currentColumn`. See the
[TableView example](../examples/components/table_view/Main.qml).

## TreeView

For `QAbstractItemModel`, set `model`; optionally set `rootIndex` to display a
subtree. Terminal-native map trees use `rootNode` and a `children` list. Keeping
these entry points separate avoids overloading Qt Quick's `model` semantics.

Properties include `model`, `rootNode`, `rootIndex`, `delegate`, `currentIndex`,
`currentRow`, `currentColumn`, and `currentItemData`. Changes to the current
node's roles notify `currentItemDataChanged`.

Methods include `expand`, `collapse`, `toggleExpanded`, `isExpanded`, `depth`,
`expandRecursively`, `collapseRecursively`, `expandToIndex`, `expandAll`, and
`collapseAll`. Expansion emits `expanded(row, depth)` or
`collapsed(row, recursively)`.

See the [TreeView example](../examples/components/tree_view/Main.qml).

## Migration from 0.1

- Replace a delegate's bare `isCurrentItem` property with the attached
  `ListView.isCurrentItem` property.
- Replace TreeView's bare `isCurrent` property with `current`; use `selected`
  when the delegate expresses selection styling.
- Move map-shaped tree data from `TreeView.model` to `TreeView.rootNode`.
- A `TableView.delegate` is now instantiated per cell. Rename an unchanged
  row-scoped delegate to `rowDelegate` as a temporary compatibility measure, or
  migrate it to consume `row` and `column`.
- Declare delegate inputs as required properties when using
  `pragma ComponentBehavior: Bound`.

The 0.2 compatibility target is the API subset documented here, not every Qt
Quick view feature. Qtui does not currently expose scene-graph transitions,
flick physics, reuse-pool callbacks, selection models, or the complete Qt Quick
positioning and item-lookup API.

## Other core elements

- `Terminal` is the root terminal surface. `rendererMode` selects frame or
  stream rendering; `append(Item)` and `print(string)` append stream output.
- `Item` is the visual base type and provides cell geometry, visibility, focus,
  clipping, anchors, and stacking order.
- `ScrollView` provides cell-based scrolling for content larger than its bounds.
- `Loader` creates one `Qtui.Item` from `source` or `sourceComponent`.
- `Row` and `Column` lay out children with integer spacing.
- `Text`, `TextInput`, and `TextArea` provide terminal text rendering and input.
- `MouseArea` provides click, wheel, hover, and drag input.

`Qtui.Controls` contains the reusable controls, while `Qtui.Extras` contains
terminal-oriented presentation components such as `BigText`, `Gauge`,
`Paragraph`, `Sparkline`, `TabHeader`, and `ToastManager`.
