# QML Types Reference

QmlTui provides 75 QML types across four QML modules. Never import `QtQuick` in a QmlTui file; build interfaces from `QtQml` and the modules below:

| QML Import | CMake Target | Notes |
| :--- | :--- | :--- |
| `import QmlTui` | `qmltui::core` | Core session, item tree, layouts, text inputs, virtualized views, input events, states, animations, and sub-cell canvas. |
| `import QmlTui.Controls` | `qmltui::controls` | Interactive buttons, selectors, indicators, panels, dialogs, tooltips, tabs, and page stacks. |
| `import QmlTui.Image` | `qmltui::image` | Raster images and sprite sheets via Kitty, Sixel, or Unicode fallback (`-DQMLTUI_ENABLE_IMAGE=ON`). |
| `import QmlTui.PtyView` | `qmltui::pty_view` | Embedded Unix pseudo-terminal on Linux and macOS (`-DQMLTUI_ENABLE_PTYVIEW=ON`). |

## `QmlTui`: Core and Terminal

- [Terminal](terminal.md): Root element managing session lifecycle, `AltScreen`/`Inline` modes, screen buffers, and input.
- [Scrollback](scrollback.md): Commits rendered lines and QML model delegates into native terminal scrollback.
- [Item](item.md): Base visual type providing integer cell geometry, anchors, visibility, focus, and states.
- [FocusScope](focusscope.md): Encapsulates keyboard focus chains inside reusable composite components.
- [Popup](popup.md): Overlay layer drawn above ordinary items with optional modal input trapping, background dimming, and focus restoration.
- [Rectangle](rectangle.md): Filled character-grid box with optional Unicode borders and linear gradients.
- [Border](border.md): Grouped property configuring border style (`Single`, `Double`, `Rounded`, `Ascii`) and color on `Rectangle`.
- [Anchors](anchors.md): Grouped property defining edge and center alignment constraints between items.
- [AnchorLine](anchorline.md): Edge or center reference line used in anchor bindings.
- [Gradient](gradient.md): Linear color gradient interpolated across cells between `GradientStop` entries.
- [GradientStop](gradientstop.md): Color stop at a normalized `[0.0, 1.0]` position inside a `Gradient`.
- [Clipboard](clipboard.md): Singleton providing OSC 52 / platform clipboard copy and Promise-based reads.
- [QmlTui](qmltui.md): Global engine singleton exposing helper factories (`span()`, `highlightRange()`, color utilities).
- [color](color.md): Terminal color value type supporting `default`, `transparent`, ANSI palette names, and RGB hex strings.
- [span](span.md): Styled inline text run passed to `Text.spans`.
- [highlightRange](highlightrange.md): Character range and style descriptor used in `TextArea` highlight layers.
- [terminalCapabilities](terminalcapabilities.md): Read-only descriptor of detected host terminal protocol support and cell dimensions.

## `QmlTui`: Layout and Containers

- [Row](row.md): Positions visible child items horizontally in a single row.
- [Column](column.md): Positions visible child items vertically in a single column.
- [RowLayout](rowlayout.md): Horizontal layout container supporting `Layout.fillWidth`, `preferredWidth`, and stretch factors.
- [ColumnLayout](columnlayout.md): Vertical layout container supporting `Layout.fillHeight`, `preferredHeight`, and stretch factors.
- [GridLayout](gridlayout.md): Two-dimensional row-and-column layout container with cell spanning and alignment.
- [StackLayout](stacklayout.md): Indexed container showing one child at a time sized to fill the layout.
- [Layout](layout.md): Attached property configuring size hints, margins, alignment, and stretch factors inside `*Layout` containers.
- [Repeater](repeater.md): Instantiates a delegate component for each entry in a model inside a parent positioner or layout.
- [Loader](loader.md): Dynamically instantiates or unloads a `Component` or QML URL on demand.

## `QmlTui`: Display and Text Inputs

- [Text](text.md): Displays plain, rich HTML, ANSI SGR, or `span`-styled text with wrapping, clipping, and link activation.
- [TextInput](textinput.md): Single-line editable text field with cursor movement, selection, echo modes, and clipboard shortcuts.
- [TextArea](textarea.md): Multi-line editable text editor with scrolling, selection, undo/redo, and syntax highlight ranges.

## `QmlTui.Controls`: Interactive Inputs, Display, and Overlays

- [AbstractButton](abstractbutton.md): Base interaction template providing `text`, `pressed`, `checkable`, `checked`, paddings, `background`, and `contentItem`.
- [Button](button.md): Focusable bordered push button emitting `clicked()` on `Enter`, `Space`, or mouse click.
- [ButtonGroup](buttongroup.md): Non-visual controller enforcing exclusive selection across `RadioButton` or checkable controls.
- [CheckBox](checkbox.md): Focusable two-state `[x]` / `[ ]` toggle option with label.
- [RadioButton](radiobutton.md): Focusable exclusive `(*)` / `( )` option coordinated with siblings via `ButtonGroup`.
- [ComboBox](combobox.md): Dropdown selector combining a single-row summary box with a modal `Popup` option list.
- [SpinBox](spinbox.md): Interactive integer stepper over `[from, to]` in `stepSize` increments.
- [Slider](slider.md): One-row horizontal numeric slider controlled via arrow keys, `Home`/`End`, mouse drag, or wheel.
- [ProgressBar](progressbar.md): One-row horizontal progress indicator filling from `from` to `to` (plus indeterminate mode).
- [BusyIndicator](busyindicator.md): Animated spinner cycling through Unicode Braille or block glyphs while `running` is true.
- [Sparkline](sparkline.md): Compact inline trend chart rendering numeric series as vertical block glyphs per column.
- [Panel](panel.md): Bordered container frame with title, trailing title, footer, and trailing footer slots on its border rows.
- [Dialog](dialog.md): Centered modal dialog box with title, content area, and standard accept/reject buttons.
- [ToolTip](tooltip.md): Attached property displaying a contextual text popup adjacent to a hovered or focused item.
- [TabBar](tabbar.md): One-row keyboard- and mouse-navigable tab strip paired with `StackLayout` or `StackView`.
- [StackView](stackview.md): Page navigation stack (`push`, `pop`, `replace`, `clear`) instantiating pages from components, URLs, or items.

## `QmlTui`: Data Views

- [TableView](tableview.md): Virtualized multi-column table with connected Unicode grid borders, column headers, and row/cell selection.
- [TreeView](treeview.md): Virtualized hierarchical view for expandable `QAbstractItemModel` trees with branch connectors.
- [ListView](listview.md): Virtualized scrollable list instantiating visible delegates from a `ListModel`, JS array, or C++ model.
- [GridView](gridview.md): Virtualized scrollable two-dimensional tile grid of model delegates.
- [ScrollView](scrollview.md): Scrollable viewport container with optional vertical and horizontal scrollbars for overflowing content.

## `QmlTui`: Input Events

- [Keys](keys.md): Attached property and signal handlers (`Keys.onPressed`) for item-level keyboard input and focus navigation.
- [KeyEvent](keyevent.md): Event payload carrying `key`, `text`, `modifiers`, and `accepted` state in `Keys` handlers.
- [MouseArea](mousearea.md): Interactive cell region handling SGR mouse clicks, drags, hover tracking, and scroll wheel events.
- [MouseEvent](mouseevent.md): Event payload carrying cell-space pointer coordinates, buttons, modifiers, and wheel deltas.
- [Shortcut](shortcut.md): Declarative keyboard shortcut (`sequence` / `sequences`) active across the application or enclosing window context.

## `QmlTui`: Animation and State

- [Animation](animation.md): Base control type (`running`, `loops`, `start()`, `stop()`) for time-driven property animations.
- [PropertyAnimation](propertyanimation.md): Animates a target property over `duration` milliseconds with configurable `Easing`.
- [NumberAnimation](numberanimation.md): Animates numeric (`int` / `real`) properties smoothly over time.
- [ColorAnimation](coloranimation.md): Interpolates RGB color properties over time.
- [Behavior](behavior.md): Applies a default animation automatically whenever a watched property value changes (`QMLTUI_ENABLE_PRIVATE_QT=ON`).
- [Easing](easing.md): Easing curve constants (`Linear`, `InOutQuad`, `OutCubic`, `OutBounce`, and more) for animations.
- [State](state.md): Named configuration of property overrides on an `Item` or `StateGroup`.
- [StateGroup](stategroup.md): Standalone state and transition machine usable outside visual `Item` hierarchies.
- [PropertyChanges](propertychanges.md): Declares target property values to apply when a `State` becomes active.
- [Transition](transition.md): Associates animations with state changes between `from` and `to` states.

## `QmlTui`: Canvas

- [Canvas](canvas.md): Sub-cell vector drawing surface rendered with 2x4 `Braille`, 1x2 `HalfBlock`, or `ASCII` characters.
- [CanvasShape](canvasshape.md): Base type for declarative 2D vector shapes placed inside a `Canvas`.
- [CanvasRectangle](canvasrectangle.md): Stroked or filled sub-cell rectangle shape on a `Canvas`.
- [Circle](circle.md): Stroked or filled sub-cell circle on a `Canvas`.
- [Line](line.md): Straight sub-cell line segment between two points on a `Canvas`.
- [Point](point.md): Individual sub-cell dot plotted on a `Canvas`.
- [Polygon](polygon.md): Connected polyline or closed polygon drawn through 2D sub-cell coordinates on a `Canvas`.

## `QmlTui.Image`

- [Image](image.md): Renders raster images (`qrc:/`, file, `http(s)`, or `ImageProvider`) via Kitty, Sixel, `HalfBlock`, `Braille`, or `ASCII`.
- [AnimatedSprite](animatedsprite.md): Plays frame-grid animations sliced from a sprite sheet image.

## `QmlTui.PtyView`

- [PtyView](ptyview.md): Hosts an interactive child process inside a Unix pseudo-terminal (`forkpty` + `libvterm`) on Linux and macOS.
