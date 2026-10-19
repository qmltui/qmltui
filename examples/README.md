# Examples

Build with `QMLTUI_BUILD_EXAMPLES=ON`; enable tests (`QMLTUI_BUILD_TESTS=ON`) to run the Runner scenarios.

## Architecture of the Examples Directory

Every directory under `examples/` has a single, non-overlapping purpose:

| Directory | Purpose |
| :--- | :--- |
| **[`gallery/`](gallery/)** | **Single source of truth for all built-in QML types** (`QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, `QmlTui.PtyView`). Contains `Main.qml` (the interactive launcher) and `examples/*.qml` (39 standalone `Item`-rooted component inspectors). Run the full grid with `qmltui_gallery`, open a tile directly with `qmltui_gallery --page Button.qml`, or launch any example standalone with `qmltui_gallery --example Button`. |
| **[`recipes/`](recipes/)** | **Copyable recipes, C++ custom items/models, & guide deep-dives.** Multi-file QML recipes (`calendar`, `status_badge`), C++ `Item` and `SyntaxHighlighter` extensions (`big_text`, `cpp_only`, `custom_cpp_item`, `text_area_highlighting`), C++ `QAbstractItemModel` / `QSortFilterProxyModel` setups (`lazy_model`, `standard_item_delegate`, `table_view`), and architectural guide companions (`independent_states`, `inline_repl`, `live_image`). |
| **[`quickstart/`](quickstart/)** & **[`wasm/`](wasm/)** | **Minimal 3-file `FetchContent` starter template** (verified in-tree by [`quickstart_test/`](quickstart_test/)) and the **browser WebAssembly playground**. |

## 1. Built-in Type Demos (`examples/gallery/examples/*.qml`)

Run any built-in type demo standalone with `./build/examples/gallery/qmltui_gallery --example <Name>` (or list all with `--list-examples`):

| Type / Concept | Standalone Gallery File | CLI Command | QML Reference |
| :--- | :--- | :--- | :--- |
| **AnimatedSprite** | [`AnimatedSprite.qml`](gallery/examples/AnimatedSprite.qml) | `qmltui_gallery --example AnimatedSprite` | [AnimatedSprite](../docs/qml-types/animatedsprite.md) |
| **Animation** | [`Animation.qml`](gallery/examples/Animation.qml) | `qmltui_gallery --example Animation` | [NumberAnimation](../docs/qml-types/numberanimation.md) |
| **Behavior** | [`Behavior.qml`](gallery/examples/Behavior.qml) | `qmltui_gallery --example Behavior` | [Behavior](../docs/qml-types/behavior.md) |
| **BusyIndicator** | [`BusyIndicator.qml`](gallery/examples/BusyIndicator.qml) | `qmltui_gallery --example BusyIndicator` | [BusyIndicator](../docs/qml-types/busyindicator.md) |
| **Button** | [`Button.qml`](gallery/examples/Button.qml) | `qmltui_gallery --example Button` | [Button](../docs/qml-types/button.md) |
| **Canvas** | [`Canvas.qml`](gallery/examples/Canvas.qml) | `qmltui_gallery --example Canvas` | [Canvas](../docs/qml-types/canvas.md) |
| **CheckBox** | [`CheckBox.qml`](gallery/examples/CheckBox.qml) | `qmltui_gallery --example CheckBox` | [CheckBox](../docs/qml-types/checkbox.md) |
| **Dialog** | [`Dialog.qml`](gallery/examples/Dialog.qml) | `qmltui_gallery --example Dialog` | [Dialog](../docs/qml-types/dialog.md) |
| **GridView** | [`GridView.qml`](gallery/examples/GridView.qml) | `qmltui_gallery --example GridView` | [GridView](../docs/qml-types/gridview.md) |
| **Image** | [`Image.qml`](gallery/examples/Image.qml) | `qmltui_gallery --example Image` | [Image](../docs/qml-types/image.md) |
| **Inline Mode** | [`InlineMode.qml`](gallery/examples/InlineMode.qml) | `qmltui_gallery --example InlineMode` | [Scrollback](../docs/qml-types/scrollback.md) |
| **Layouts** | [`Layouts.qml`](gallery/examples/Layouts.qml) | `qmltui_gallery --example Layouts` | [RowLayout](../docs/qml-types/rowlayout.md) |
| **ListView** | [`ListView.qml`](gallery/examples/ListView.qml) | `qmltui_gallery --example ListView` | [ListView](../docs/qml-types/listview.md) |
| **Loader** | [`Loader.qml`](gallery/examples/Loader.qml) | `qmltui_gallery --example Loader` | [Loader](../docs/qml-types/loader.md) |
| **MouseArea** | [`MouseArea.qml`](gallery/examples/MouseArea.qml) | `qmltui_gallery --example MouseArea` | [MouseArea](../docs/qml-types/mousearea.md) |
| **Panel** | [`Panel.qml`](gallery/examples/Panel.qml) | `qmltui_gallery --example Panel` | [Panel](../docs/qml-types/panel.md) |
| **Popup** | [`Popup.qml`](gallery/examples/Popup.qml) | `qmltui_gallery --example Popup` | [Popup](../docs/qml-types/popup.md) |
| **ProgressBar** | [`ProgressBar.qml`](gallery/examples/ProgressBar.qml) | `qmltui_gallery --example ProgressBar` | [ProgressBar](../docs/qml-types/progressbar.md) |
| **PtyView** | [`PtyView.qml`](gallery/examples/PtyView.qml) | `qmltui_gallery --example PtyView` | [PtyView](../docs/qml-types/ptyview.md) |
| **RadioButton** | [`RadioButton.qml`](gallery/examples/RadioButton.qml) | `qmltui_gallery --example RadioButton` | [RadioButton](../docs/qml-types/radiobutton.md) |
| **Rectangle** | [`Rectangle.qml`](gallery/examples/Rectangle.qml) | `qmltui_gallery --example Rectangle` | [Rectangle](../docs/qml-types/rectangle.md) |
| **Repeater** | [`Repeater.qml`](gallery/examples/Repeater.qml) | `qmltui_gallery --example Repeater` | [Repeater](../docs/qml-types/repeater.md) |
| **RichText** | [`RichText.qml`](gallery/examples/RichText.qml) | `qmltui_gallery --example RichText` | [Text](../docs/qml-types/text.md) |
| **ScrollView** | [`ScrollView.qml`](gallery/examples/ScrollView.qml) | `qmltui_gallery --example ScrollView` | [ScrollView](../docs/qml-types/scrollview.md) |
| **Shortcut** | [`Shortcut.qml`](gallery/examples/Shortcut.qml) | `qmltui_gallery --example Shortcut` | [Shortcut](../docs/qml-types/shortcut.md) |
| **Slider** | [`Slider.qml`](gallery/examples/Slider.qml) | `qmltui_gallery --example Slider` | [Slider](../docs/qml-types/slider.md) |
| **Sparkline** | [`Sparkline.qml`](gallery/examples/Sparkline.qml) | `qmltui_gallery --example Sparkline` | [Sparkline](../docs/qml-types/sparkline.md) |
| **StackView** | [`StackView.qml`](gallery/examples/StackView.qml) | `qmltui_gallery --example StackView` | [StackView](../docs/qml-types/stackview.md) |
| **States** | [`States.qml`](gallery/examples/States.qml) | `qmltui_gallery --example States` | [State](../docs/qml-types/state.md) |
| **Switch** | [`Switch.qml`](gallery/examples/Switch.qml) | `qmltui_gallery --example Switch` | [Switch](../docs/qml-types/switch.md) |
| **TabBar** | [`TabBar.qml`](gallery/examples/TabBar.qml) | `qmltui_gallery --example TabBar` | [TabBar](../docs/qml-types/tabbar.md) |
| **TableView** | [`TableView.qml`](gallery/examples/TableView.qml) | `qmltui_gallery --example TableView` | [TableView](../docs/qml-types/tableview.md) |
| **TerminalCapabilities** | [`TerminalCapabilities.qml`](gallery/examples/TerminalCapabilities.qml) | `qmltui_gallery --example TerminalCapabilities` | [TerminalCapabilities](../docs/qml-types/terminalcapabilities.md) |
| **Text** | [`Text.qml`](gallery/examples/Text.qml) | `qmltui_gallery --example Text` | [Text](../docs/qml-types/text.md) |
| **TextArea** | [`TextArea.qml`](gallery/examples/TextArea.qml) | `qmltui_gallery --example TextArea` | [TextArea](../docs/qml-types/textarea.md) |
| **TextInput** | [`TextInput.qml`](gallery/examples/TextInput.qml) | `qmltui_gallery --example TextInput` | [TextInput](../docs/qml-types/textinput.md) |
| **Toast** | [`Toast.qml`](gallery/examples/Toast.qml) | `qmltui_gallery --example Toast` | [Toast](../docs/qml-types/toast.md) |
| **ToolTip** | [`ToolTip.qml`](gallery/examples/ToolTip.qml) | `qmltui_gallery --example ToolTip` | [ToolTip](../docs/qml-types/tooltip.md) |
| **TreeView** | [`TreeView.qml`](gallery/examples/TreeView.qml) | `qmltui_gallery --example TreeView` | [TreeView](../docs/qml-types/treeview.md) |

## 2. Reusable Recipes, C++ Models, & Guide Examples (`examples/recipes/`)

| Example | CMake Target | Related Guide | What It Demonstrates |
| :--- | :--- | :--- | :--- |
| **[`big_text/`](recipes/big_text/)** | `qmltui_big_text_demo` | [Custom C++ Items](../docs/guides/custom-items.md) | Custom C++ `Item` subclass (`BigText.h/.cpp`) painting block-pixel banner text into `TextGridBuffer`. |
| **[`calendar/`](recipes/calendar/)** | `qmltui_calendar_demo` | [Layouts](../docs/guides/layouts.md) | Copyable pure-QML `Calendar.qml` (`Column`/`Row`/`Repeater` month grid) + `StatusBadge.qml` planner recipe with themes and notes. |
| **[`cpp_only/`](recipes/cpp_only/)** | `qmltui_cpp_only_demo` | [Custom C++ Items](../docs/guides/custom-items.md) | Building a `qmltui::Terminal` and reactive item tree purely in C++ without `.qml` files. |
| **[`custom_cpp_item/`](recipes/custom_cpp_item/)** | `qmltui_custom_cpp_item_demo` | [Custom C++ Items](../docs/guides/custom-items.md) | Subclassing `qmltui::Item`, overriding `paint(TextGridBuffer &, int, int, const ClipRect &)`, and registering with `QML_ELEMENT`. |
| **[`independent_states/`](recipes/independent_states/)** | `qmltui_independent_states_demo` | [Animation & States](../docs/guides/animation-and-states.md) | Orthogonal state machines using `StateGroup`, `State`, and `Transition` (`QMLTUI_ENABLE_PRIVATE_QT=ON`). |
| **[`inline_repl/`](recipes/inline_repl/)** | `qmltui_inline_repl_demo` | [Inline Mode](../docs/guides/inline-mode.md) | Minimal command-line REPL using `Terminal.Inline` and `Scrollback.print()`. |
| **[`lazy_model/`](recipes/lazy_model/)** | `qmltui_lazy_model_demo` | [Models & Views](../docs/guides/models.md) | `LazyDirectoryModel` (`QAbstractItemModel`) populating a lazy 4-level directory tree on demand inside `TreeView`. |
| **[`live_image/`](recipes/live_image/)** | `qmltui_live_image_demo` | [Architecture](../docs/guides/architecture.md) | Streaming procedural `QImage` frames from `qmltui::AsyncImageProvider` into a terminal `Image` (`QMLTUI_ENABLE_IMAGE=ON`). |
| **[`standard_item_delegate/`](recipes/standard_item_delegate/)** | `qmltui_standard_item_delegate_demo` | [Models & Views](../docs/guides/models.md) | Shared C++ `ReleaseChecklistModel` (`QStandardItemModel`) rendered with `StandardItemDelegate` in `ListView` alongside `TableView` and `TreeView`, plus a side-by-side Qt Widgets target (`qmltui_standard_item_delegate_widgets`). |
| **[`status_badge/`](recipes/status_badge/)** | `qmltui_status_badge_demo` | [Styling & Colors](../docs/guides/styling-and-colors.md) | Copyable pure-QML status pill component (`StatusBadge.qml`). |
| **[`table_view/`](recipes/table_view/)** | `qmltui_table_view_demo` | [Models & Views](../docs/guides/models.md) | C++ `ProcessModel` (`QAbstractTableModel`) and `ProcessProxy` (`QSortFilterProxyModel`) sorting and live filtering over a `TableView`. |
| **[`text_area_highlighting/`](recipes/text_area_highlighting/)** | `qmltui_text_area_highlighting_demo` | [Styling & Colors](../docs/guides/styling-and-colors.md) | Custom syntax highlighting on `TextArea` in both C++ (`JsonHighlighter`) and QML (`QmlHighlighter`), layered with live search highlights. |

## 3. Quickstart & WebAssembly (`examples/quickstart/`, `examples/wasm/`)

- [Standalone quickstart](quickstart/) and [its in-tree Runner test](quickstart_test/)
- [WebAssembly playground](wasm/) ([Guide](../docs/guides/webassembly.md))
