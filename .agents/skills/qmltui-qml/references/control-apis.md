# QmlTui High-Divergence Control & Item API Quick Reference

On-demand property, signal, and method reference for QmlTui types whose APIs differ from Qt Quick. For full contracts, inspect `docs/qml-types/` (in `FetchContent` consumer builds: `build/_deps/qmltui-src/docs/qml-types/`) or generated `build/qml/QmlTui*/*.qmltypes`.

## Data Views & Scrollback (`import QmlTui`)

### `TableView`
- **Core properties**: `model` (2D JS array, `TableModel`, or `QAbstractItemModel`), `columns` (list of `{ title, key, role, width, stretch, align, headerAlign, padding, leftPadding, rightPadding, color, background }`), `selectedRow` (default `-1`; non-negative values assigned before `model` resolves are retained and clamped once rows load), `currentColumn`, `stretchLastColumn`, `alternatingRows`, `selectionKeepsCellColors`.
- **Connected frame & grid** (no `Rectangle.border` group): `showOuterBorder`, `showColumnGrid`, `showRowGrid` (all default `false`), `gridStyle` (`TableView.Single`, `TableView.Double`, `TableView.Ascii`), `gridColor`.
- **Styling & formatting**: `headerColor`, `headerBackgroundColor`, `selectionColor`, `selectedTextColor`, `textColor`, `rowBackgroundColor`, `alternatingRowBackgroundColor`, `cellFormat: (row, column, value) => span | list<span> | { text, spans, color, backgroundColor, bold, dim, italic, underline, reversed, align, padding, leftPadding, rightPadding }`.
- **Custom delegates**: Instantiated per `(row, column)` cell with `index`, `row`, `column`, `current`, `selected`, `display`, and C++ model role names.
- **Signals / methods**: `rowActivated(int index)`, `headerClicked(int column)`, `forceLayout()`.

### `ListView`
- **Properties**: `model`, `delegate` (default), `orientation` (`Qt.Vertical` | `Qt.Horizontal`), `spacing`, `count` (read-only), `currentIndex` (retains non-negative values set before `model` loads), `reuseItems` (default `true`), `rootIndex`.
- **Attached properties**: `ListView.view`, `ListView.isCurrentItem`; attached signals `ListView.pooled()`, `ListView.reused()`.
- **Signals / methods**: `itemActivated(int index)`, `modelDataChanged()`, `itemData(int index, string role = "")`.

### `Scrollback` (non-visual, `Terminal.Inline`)
- **Properties**: `model`, `delegate` (default), `spacing`, `separator`, `reuseItems`.
- **Methods**:
  - `print(value)` -> `bool`: bypasses `delegate`/`spacing`/`separator`; accepts `string`, `span` (`QmlTui.span()`), `{ text, color, bold, ... }` map, or an array of spans/maps.
  - `append(data)` / `append(Item)` / `append(span)` / `append(list<span>)`: renders via `delegate` (or snapshots `Item`/spans) honoring `spacing` and `separator`. In a `delegate`, declare `required property var model` (and bind via an explicit `id` on the delegate root under `pragma ComponentBehavior: Bound`, not `parent.model`).
  - `clear()`.
- **Signals**: `outputRejected(string reason)` (ANSI/UTF-8 byte queue bounded to 1 MiB by default; `QMLTUI_SCROLLBACK_QUEUE_LIMIT`).

## Inputs & Shortcuts (`import QmlTui`)

### `TextInput`
- **Properties**: `text`, `color`, `placeholderText`, `placeholderColor`, `displayText`, `readOnly`, `maximumLength`, `validator`, `echoMode` (`TextInput.Normal` | `Password` | `NoEcho`), `echoCharacter`, `cursorShape` (`Block` | `Underline` | `Beam` | `HollowBlock`), `cursorColor`, `cursorVisible`, `cursorBlinking` (default `false` — enabling it schedules periodic repaints while focused), `cursorBlinkRate`, `selectionColor`, `selectedTextColor`.
- **Signals / methods**: `accepted()`, `editingFinished()`, `selectionChanged()`, `clear()`, `selectAll()`, `select(start, end)`, `copy()`, `cut()`, `paste(text)`, `undo()`, `redo()`.

### `Shortcut`
- **Properties**: `sequence` (portable chord string `"Ctrl+Q"`, `"Ctrl+Shift+P"`, `"Alt+Return"`, `"F1"`..`"F12"`, or Qt bitmask `Qt.ControlModifier | Qt.Key_Q`), `enabled`.
- **Signals**: `activated()`.

## Controls & Overlays (`import QmlTui.Controls` & `import QmlTui`)

### `Panel` (`QmlTui.Controls`, inherits `Rectangle`)
- **Border slots**: `title`, `titleTrailing`, `footer`, `footerTrailing` (each takes an `Item`, e.g., `Text` or `Row`), `titleAlignment` (`Qt.AlignLeft` | `Qt.AlignHCenter` | `Qt.AlignRight`).
- **Auto-inset**: Visible `border.style !== Border.None` automatically adds 1 cell to `leftPadding`/`rightPadding`/`topPadding`/`bottomPadding`, so `anchors.fill: parent` already stays inside the border.

### `TabBar` (`QmlTui.Controls`, borderless `Item`)
- **Properties**: `tabs` (JS string array `["Overview", "Logs"]` — not child `TabButton` items), `currentIndex` (clamped; values set before `tabs` arrive are preserved), `color`, `textColor`, `selectionColor`, `selectedTextColor`.
- **Behavior**: Left/Right arrows move `currentIndex` when focused; mouse clicks select and focus. Bind `StackLayout.currentIndex: tabBar.currentIndex`.

### `Button` (`QmlTui.Controls`, inherits `AbstractButton`)
- **Properties**: `text`, `textColor`, `color`, `borderColor`, `down`, `pressed`, `hovered`, `highlighted`, `checkable`, `checked`, `leftPadding` (default `2`), `rightPadding` (default `2`), `background`, `contentItem`.
- **Cell budget**: Default inner label width is `width - 4` (border + inner space per side); set `leftPadding: 1; rightPadding: 1` for `width - 2`.

### `Sparkline` (`QmlTui.Controls`, borderless `Item`)
- **Properties**: `values` (JS array of numbers; newest value on the right), `from`, `to` (`0.0` auto-scales to min/max), `filled` (`bool`), `color`, `backgroundColor`.

### `ProgressBar` (`QmlTui.Controls`)
- **Properties**: `value`, `from` (`0.0`), `to` (`1.0`), `showBrackets` (`bool`, consumes 2 cells), `showPercentage` (`bool`, consumes 5 cells), `style` (`"block"`, `"segmented"`, `"line"`, `"pill"`, `"dot"`, `"hash"`), `color`, `trackColor`, `fillCharacter`, `emptyCharacter`, `gradient`, `thresholds` (`[{ value, color }, ...]`).

### `BusyIndicator` (`QmlTui.Controls`, inherits `Text`)
- **Properties**: `running` (default `true` — guard with `running: visible && isBusy` so idle screens do not repaint), `interval` (`100` ms), `frames` (`list<string>`).

### `Popup` (`QmlTui`) & `Dialog` (`QmlTui.Controls`, inherits `Popup`)
- **`Popup`**: `modal`, `dimBackground` (default `true` when modal), `closePolicy` (`Popup.NoAutoClose` | `Popup.CloseOnEscape` | `Popup.CloseOnOutsideClick`), `initialFocusItem`, `owner`; methods `open()`, `close()`, `accept()`, `reject()`; signals `opened()`, `closed()`, `accepted()`, `rejected()`, `finished(bool accepted)`. Always prefer `open()`/`close()` over setting `visible` directly so focus is saved and restored.
- **`Dialog`**: Preconfigured modal centered `Popup` with `title` (`string`), `message` (`string`), `acceptText` (`"OK"`), `rejectText` (`"Cancel"`), `showRejectButton`, `textColor`, `titleColor`, and read-only `background` (`Rectangle`).
