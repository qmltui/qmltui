# Examples and Recipes

Configure QmlTui with `-DQMLTUI_BUILD_EXAMPLES=ON` to build the interactive component gallery and standalone recipe executables:

```sh
cmake -S . -B build -G Ninja -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build -j8 --target qmltui_gallery

# Open the interactive gallery grid:
./build/examples/gallery/qmltui_gallery

# Open the gallery on a specific page:
./build/examples/gallery/qmltui_gallery --page Button.qml

# Run a single gallery example standalone:
./build/examples/gallery/qmltui_gallery --example Button

# List all gallery example names:
./build/examples/gallery/qmltui_gallery --list-examples
```

![QmlTui Interactive Gallery](assets/casts/gallery-tour.gif)

## Controls vs. Recipes

`QmlTui.Controls` only includes types that depend on engine internals an application cannot reproduce in QML (cell-grid geometry, grapheme width measurement, viewport virtualization, or overlay z-stacking) or that are standard across terminal toolkits. Higher-level or domain-specific components (`Calendar`, `StatusBadge`, `BigText`, syntax highlighters, and C++ model integrations) live as BSD-3-Clause recipes under [`examples/recipes/`](../examples/recipes/) so applications can copy and adapt them directly.

## Component Gallery (`examples/gallery/examples/`)

Each file in [`examples/gallery/examples/`](../examples/gallery/examples/) is a self-contained `Item`-rooted QML component loaded by `qmltui_gallery`:

| Example | Source File | CLI Command | QML Reference | Recording |
| :--- | :--- | :--- | :--- | :--- |
| **Text** | [`Text.qml`](../examples/gallery/examples/Text.qml) | `--example Text` | [Text](qml-types/text.md) | [GIF](assets/casts/components/text.gif) |
| **RichText** | [`RichText.qml`](../examples/gallery/examples/RichText.qml) | `--example RichText` | [Text](qml-types/text.md) | [GIF](assets/casts/components/rich_text.gif) |
| **Rectangle** | [`Rectangle.qml`](../examples/gallery/examples/Rectangle.qml) | `--example Rectangle` | [Rectangle](qml-types/rectangle.md) | [GIF](assets/casts/components/rectangle.gif) |
| **Color** | [`Color.qml`](../examples/gallery/examples/Color.qml) | `--example Color` | [color](qml-types/color.md) / [QmlTui](qml-types/qmltui.md) | - |
| **Gradient** | [`Gradient.qml`](../examples/gallery/examples/Gradient.qml) | `--example Gradient` | [Gradient](qml-types/gradient.md) | - |
| **TextInput** | [`TextInput.qml`](../examples/gallery/examples/TextInput.qml) | `--example TextInput` | [TextInput](qml-types/textinput.md) | [GIF](assets/casts/components/text_input.gif) |
| **TextArea** | [`TextArea.qml`](../examples/gallery/examples/TextArea.qml) | `--example TextArea` | [TextArea](qml-types/textarea.md) | [GIF](assets/casts/components/text_area.gif) |
| **MouseArea** | [`MouseArea.qml`](../examples/gallery/examples/MouseArea.qml) | `--example MouseArea` | [MouseArea](qml-types/mousearea.md) | [GIF](assets/casts/components/mouse_area.gif) |
| **Keys** | [`Keys.qml`](../examples/gallery/examples/Keys.qml) | `--example Keys` | [Keys](qml-types/keys.md) | - |
| **Anchors** | [`Anchors.qml`](../examples/gallery/examples/Anchors.qml) | `--example Anchors` | [Anchors](qml-types/anchors.md) | - |
| **Layouts** | [`Layouts.qml`](../examples/gallery/examples/Layouts.qml) | `--example Layouts` | [RowLayout](qml-types/rowlayout.md) / [GridLayout](qml-types/gridlayout.md) | [GIF](assets/casts/components/layouts.gif) |
| **StackLayout** | [`StackLayout.qml`](../examples/gallery/examples/StackLayout.qml) | `--example StackLayout` | [StackLayout](qml-types/stacklayout.md) | - |
| **Shortcut** | [`Shortcut.qml`](../examples/gallery/examples/Shortcut.qml) | `--example Shortcut` | [Shortcut](qml-types/shortcut.md) | [GIF](assets/casts/components/shortcut.gif) |
| **FocusScope** | [`FocusScope.qml`](../examples/gallery/examples/FocusScope.qml) | `--example FocusScope` | [FocusScope](qml-types/focusscope.md) | [GIF](assets/casts/components/focus_scope.gif) |
| **Repeater** | [`Repeater.qml`](../examples/gallery/examples/Repeater.qml) | `--example Repeater` | [Repeater](qml-types/repeater.md) | [GIF](assets/casts/components/repeater.gif) |
| **Loader** | [`Loader.qml`](../examples/gallery/examples/Loader.qml) | `--example Loader` | [Loader](qml-types/loader.md) | [GIF](assets/casts/components/loader.gif) |
| **ListView** | [`ListView.qml`](../examples/gallery/examples/ListView.qml) | `--example ListView` | [ListView](qml-types/listview.md) | [GIF](assets/casts/components/list_view.gif) |
| **TableView** | [`TableView.qml`](../examples/gallery/examples/TableView.qml) | `--example TableView` | [TableView](qml-types/tableview.md) | [GIF](assets/casts/components/table_view.gif) |
| **TreeView** | [`TreeView.qml`](../examples/gallery/examples/TreeView.qml) | `--example TreeView` | [TreeView](qml-types/treeview.md) | [GIF](assets/casts/components/tree_view.gif) |
| **GridView** | [`GridView.qml`](../examples/gallery/examples/GridView.qml) | `--example GridView` | [GridView](qml-types/gridview.md) | [GIF](assets/casts/components/grid_view.gif) |
| **ScrollView** | [`ScrollView.qml`](../examples/gallery/examples/ScrollView.qml) | `--example ScrollView` | [ScrollView](qml-types/scrollview.md) | [GIF](assets/casts/components/scroll_view.gif) |
| **Popup** | [`Popup.qml`](../examples/gallery/examples/Popup.qml) | `--example Popup` | [Popup](qml-types/popup.md) | [GIF](assets/casts/components/popup.gif) |
| **Clipboard** | [`Clipboard.qml`](../examples/gallery/examples/Clipboard.qml) | `--example Clipboard` | [Clipboard](qml-types/clipboard.md) | [GIF](assets/casts/components/clipboard.gif) |
| **Canvas** | [`Canvas.qml`](../examples/gallery/examples/Canvas.qml) | `--example Canvas` | [Canvas](qml-types/canvas.md) | [GIF](assets/casts/components/canvas.gif) |
| **States** | [`States.qml`](../examples/gallery/examples/States.qml) | `--example States` | [State](qml-types/state.md) / [Transition](qml-types/transition.md) | [GIF](assets/casts/components/states.gif) |
| **Animation** | [`Animation.qml`](../examples/gallery/examples/Animation.qml) | `--example Animation` | [NumberAnimation](qml-types/numberanimation.md) | [GIF](assets/casts/components/animation.gif) |
| **Behavior** | [`Behavior.qml`](../examples/gallery/examples/Behavior.qml) | `--example Behavior` | [Behavior](qml-types/behavior.md) | [GIF](assets/casts/components/behavior.gif) |
| **Button** | [`Button.qml`](../examples/gallery/examples/Button.qml) | `--example Button` | [Button](qml-types/button.md) | [GIF](assets/casts/components/button.gif) |
| **CheckBox** | [`CheckBox.qml`](../examples/gallery/examples/CheckBox.qml) | `--example CheckBox` | [CheckBox](qml-types/checkbox.md) | [GIF](assets/casts/components/check_box.gif) |
| **RadioButton** | [`RadioButton.qml`](../examples/gallery/examples/RadioButton.qml) | `--example RadioButton` | [RadioButton](qml-types/radiobutton.md) | [GIF](assets/casts/components/radio_button.gif) |
| **ComboBox** | [`ComboBox.qml`](../examples/gallery/examples/ComboBox.qml) | `--example ComboBox` | [ComboBox](qml-types/combobox.md) | [GIF](assets/casts/components/combo_box.gif) |
| **SpinBox** | [`SpinBox.qml`](../examples/gallery/examples/SpinBox.qml) | `--example SpinBox` | [SpinBox](qml-types/spinbox.md) | [GIF](assets/casts/components/spin_box.gif) |
| **Slider** | [`Slider.qml`](../examples/gallery/examples/Slider.qml) | `--example Slider` | [Slider](qml-types/slider.md) | [GIF](assets/casts/components/slider.gif) |
| **ProgressBar** | [`ProgressBar.qml`](../examples/gallery/examples/ProgressBar.qml) | `--example ProgressBar` | [ProgressBar](qml-types/progressbar.md) | [GIF](assets/casts/components/progress_bar.gif) |
| **Panel** | [`Panel.qml`](../examples/gallery/examples/Panel.qml) | `--example Panel` | [Panel](qml-types/panel.md) | [GIF](assets/casts/components/panel.gif) |
| **Dialog** | [`Dialog.qml`](../examples/gallery/examples/Dialog.qml) | `--example Dialog` | [Dialog](qml-types/dialog.md) | [GIF](assets/casts/components/dialog.gif) |
| **TabBar** | [`TabBar.qml`](../examples/gallery/examples/TabBar.qml) | `--example TabBar` | [TabBar](qml-types/tabbar.md) | [GIF](assets/casts/components/tab_bar.gif) |
| **StackView** | [`StackView.qml`](../examples/gallery/examples/StackView.qml) | `--example StackView` | [StackView](qml-types/stackview.md) | [GIF](assets/casts/components/stack_view.gif) |
| **BusyIndicator** | [`BusyIndicator.qml`](../examples/gallery/examples/BusyIndicator.qml) | `--example BusyIndicator` | [BusyIndicator](qml-types/busyindicator.md) | [GIF](assets/casts/components/busy_indicator.gif) |
| **ToolTip** | [`ToolTip.qml`](../examples/gallery/examples/ToolTip.qml) | `--example ToolTip` | [ToolTip](qml-types/tooltip.md) | [GIF](assets/casts/components/tool_tip.gif) |
| **Sparkline** | [`Sparkline.qml`](../examples/gallery/examples/Sparkline.qml) | `--example Sparkline` | [Sparkline](qml-types/sparkline.md) | [GIF](assets/casts/components/sparkline.gif) |
| **Image** | [`Image.qml`](../examples/gallery/examples/Image.qml) | `--example Image` | [Image](qml-types/image.md) | [GIF](assets/casts/components/image.gif) |
| **AnimatedSprite** | [`AnimatedSprite.qml`](../examples/gallery/examples/AnimatedSprite.qml) | `--example AnimatedSprite` | [AnimatedSprite](qml-types/animatedsprite.md) | [GIF](assets/casts/components/animated_sprite.gif) |
| **PtyView** | [`PtyView.qml`](../examples/gallery/examples/PtyView.qml) | `--example PtyView` | [PtyView](qml-types/ptyview.md) | [GIF](assets/casts/components/pty_view.gif) |
| **TerminalCapabilities** | [`TerminalCapabilities.qml`](../examples/gallery/examples/TerminalCapabilities.qml) | `--example TerminalCapabilities` | [TerminalCapabilities](qml-types/terminalcapabilities.md) | [GIF](assets/casts/components/terminal_capabilities.gif) |

## Standalone Recipes and C++ Examples (`examples/recipes/`)

[`examples/recipes/`](../examples/recipes/) contains multi-file QML components, custom C++ `Item` and `SyntaxHighlighter` subclasses, `QAbstractItemModel` / `QSortFilterProxyModel` setups, and companion executables for `docs/guides/`:

| Recipe | Directory | CMake Target | Guide / Recording | Description |
| :--- | :--- | :--- | :--- | :--- |
| **BigText** | [`examples/recipes/big_text/`](../examples/recipes/big_text/) | `qmltui_big_text_demo` | [Custom C++ Items](guides/custom-items.md) · [GIF](assets/casts/components/big_text.gif) | Custom C++ `qmltui::Item` (`BigText.h`, `BigText.cpp`) painting 3x5 block-pixel banner text into `TextGridBuffer`. |
| **Calendar** | [`examples/recipes/calendar/`](../examples/recipes/calendar/) | `qmltui_calendar_demo` | [Layouts](guides/layouts.md) · [GIF](assets/casts/components/calendar.gif) | Reusable `Calendar.qml` month-grid date picker built with `Column`, `Row`, `Repeater`, and keyboard navigation. |
| **C++-Only Application** | [`examples/recipes/cpp_only/`](../examples/recipes/cpp_only/) | `qmltui_cpp_only_demo` | [Custom C++ Items](guides/custom-items.md) | Constructing and updating a `qmltui::Terminal` and item tree directly from C++ without `.qml` files. |
| **Custom C++ Item** | [`examples/recipes/custom_cpp_item/`](../examples/recipes/custom_cpp_item/) | `qmltui_custom_cpp_item_demo` | [Custom C++ Items](guides/custom-items.md) | Subclassing `qmltui::Rectangle` (`ProgressBar`), overriding `paint(TextGridBuffer &, int, int, const ClipRect &)`, and registering with `QML_ELEMENT`. |
| **Independent States** | [`examples/recipes/independent_states/`](../examples/recipes/independent_states/) | `qmltui_independent_states_demo` | [Animation & States](guides/animation-and-states.md) | Orthogonal state machines using `StateGroup`, `State`, and `Transition` (`QMLTUI_ENABLE_PRIVATE_QT=ON`). |
| **Inline REPL** | [`examples/recipes/inline_repl/`](../examples/recipes/inline_repl/) | `qmltui_inline_repl_demo` | [Inline Mode](guides/inline-mode.md) · [GIF](assets/casts/components/inline_mode.gif) | Command-line REPL using `Terminal.Inline`, `Scrollback`, and `ProgressBar`. |
| **Lazy Model** | [`examples/recipes/lazy_model/`](../examples/recipes/lazy_model/) | `qmltui_lazy_model_demo` | [Models & Views](guides/models.md) | C++ `LazyDirectoryModel` (`QAbstractItemModel`) populating a 4-level directory tree on demand in `TreeView`. |
| **Live Image** | [`examples/recipes/live_image/`](../examples/recipes/live_image/) | `qmltui_live_image_demo` | [Architecture](guides/architecture.md) | Streaming procedural `QImage` frames from `qmltui::AsyncImageProvider` into `Image` (`QMLTUI_ENABLE_IMAGE=ON`). |
| **PySide6** | [`examples/recipes/pyside6/`](../examples/recipes/pyside6/) | *(Python script)* | [Models & Views](guides/models.md) | Hosting `QmlTui` and `QmlTui.Controls` from Python with `PySide6`, a Python `QAbstractTableModel`, and `QSortFilterProxyModel`. |
| **StandardItemDelegate** | [`examples/recipes/standard_item_delegate/`](../examples/recipes/standard_item_delegate/) | `qmltui_standard_item_delegate_demo` / `qmltui_standard_item_delegate_widgets` | [Models & Views](guides/models.md) · [GIF](assets/casts/components/standard_item_delegate.gif) | Shared C++ `ReleaseChecklistModel` (`QStandardItemModel`) rendered with `StandardItemDelegate` in `ListView` alongside `TableView`, `TreeView`, and an optional Qt Widgets window. |
| **StatusBadge** | [`examples/recipes/status_badge/`](../examples/recipes/status_badge/) | `qmltui_status_badge_demo` | [Styling & Colors](guides/styling-and-colors.md) · [GIF](assets/casts/components/status_badge.gif) | Copyable `StatusBadge.qml` pill component (`[OK]`, `[WARN]`, `[ERR]`) with configurable icon, label, border, and color presets. |
| **TableView Model** | [`examples/recipes/table_view/`](../examples/recipes/table_view/) | `qmltui_table_view_demo` | [Models & Views](guides/models.md) · [GIF](assets/casts/components/table_view.gif) | `TableView` backed by C++ `ProcessModel` (`QAbstractTableModel`) and `ProcessProxy` (`QSortFilterProxyModel`) with column sorting and filtering. |
| **TextArea Highlighting** | [`examples/recipes/text_area_highlighting/`](../examples/recipes/text_area_highlighting/) | `qmltui_text_area_highlighting_demo` | [Styling & Colors](guides/styling-and-colors.md) · [GIF](assets/casts/components/text_area_highlighting.gif) | Custom syntax highlighting on `TextArea` implemented in both QML (`QmlHighlighter.qml`) and C++ (`JsonHighlighter`). |

## Starter Template (`examples/helloworld/`)

| Project | Directory | CMake Target | Description |
| :--- | :--- | :--- | :--- |
| **Standalone Hello World** | [`examples/helloworld/`](../examples/helloworld/) | `hello_qmltui` (`hello_world` Runner scenario) | Minimal 3-file (`CMakeLists.txt`, `main.cpp`, `Main.qml`) starter project supporting `FetchContent`, `find_package(QmlTui)`, and `qmltui_wasm_app()`. |

## Official Add-on Modules

Opinionated higher-level UI toolkits built on `QmlTui` and `QmlTui.Controls` live in standalone repositories consumable via CMake `FetchContent`:

| Module | Repository & Target | Showcase App | Description |
| :--- | :--- | :--- | :--- |
| **`QmlTui.TVision`** | [`qmltui/tvision`](https://github.com/qmltui/tvision) (`QmlTui::TVision`) | [`turbo_hex`](https://github.com/qmltui/apps/tree/main/apps/turbo_hex) | Borland Turbo Vision-inspired Multi-Document Interface (`TvDesktop`, draggable/resizable `TvWindow`s, `TvMenuBar` mnemonics, `TvDialog`, 3D beveled `TvButton`, per-file `TvPalette` themes, and glyph-preserving `TvDropShadow`). |

## Showcase Applications (`qmltui/apps`)

Complete, multi-panel terminal applications built with QmlTui live in the companion [qmltui/apps](https://github.com/qmltui/apps) repository:

| Application | Source | Key QmlTui Features |
| :--- | :--- | :--- |
| **agent_stream** | [`apps/agent_stream`](https://github.com/qmltui/apps/tree/main/apps/agent_stream) | `Terminal.Inline`, `Scrollback`, slash-command autocomplete, diff inspector, streaming tool cards. |
| **package_manager** | [`apps/package_manager`](https://github.com/qmltui/apps/tree/main/apps/package_manager) | Multi-stage `Terminal.Inline` CLI installer (`qpm`) with `TableView`, `TreeView`, `Sparkline`, and `Scrollback` receipts. |
| **qtop** | [`apps/qtop`](https://github.com/qmltui/apps/tree/main/apps/qtop) | [`btop++`](https://github.com/aristocratos/btop)-inspired UI layout study in ~770 lines of QML (`Canvas.Braille` CPU history, core meters, sparklines, sortable `TableView`, and `Behavior` animations). |
| **container_fleet** | [`apps/container_fleet`](https://github.com/qmltui/apps/tree/main/apps/container_fleet) | Fleet dashboard with `TableView` sorting, `TabBar` status filters, live search, and streaming log drawer. |
| **file_manager** | [`apps/file_manager`](https://github.com/qmltui/apps/tree/main/apps/file_manager) | Miller-column browser with syntax-highlighted code/Markdown previews, `Image` rendering, and an embedded `PtyView` shell drawer. |
| **markdown_viewer** | [`apps/markdown_viewer`](https://github.com/qmltui/apps/tree/main/apps/markdown_viewer) | AST-driven Markdown reader (`md4qt`) with Unicode box tables, heading navigation, search highlights, and live style editor. |
| **turbo_hex** | [`apps/turbo_hex`](https://github.com/qmltui/apps/tree/main/apps/turbo_hex) | Multi-window Hex & Binary Inspector built on [`QmlTui.TVision`](https://github.com/qmltui/tvision) with `BoundedSelfPaint` Hex + ASCII views, Little/Big-Endian Data Inspector, and ELF/PNG/ZIP/WASM header & string span highlighting. |
| **audio_player** | [`apps/audio_player`](https://github.com/qmltui/apps/tree/main/apps/audio_player) | Terminal audio player with 24-band FFT spectrum analyzer, `Canvas.Braille` stereo oscilloscope, playlist, and ANSI skins. |
| **plotting_dashboard** | [`apps/plotting_dashboard`](https://github.com/qmltui/apps/tree/main/apps/plotting_dashboard) | Real-time `Canvas.Braille` waveform oscilloscope and XY Lissajous phase orbits. |
| **industrial_plc_hmi** | [`apps/industrial_plc_hmi`](https://github.com/qmltui/apps/tree/main/apps/industrial_plc_hmi) | SCADA/PLC operator console with telemetry gauges, trend chart, alarm log, hardware diagnostics, and safety interlocks. |
| **screensaver** | [`apps/screensaver`](https://github.com/qmltui/apps/tree/main/apps/screensaver) | 11 demoscene rendering modes (`Canvas.HalfBlock`, `Canvas.Braille`, 3D box pipes, raymarched donut, Doom Fire, Lorenz attractor). |
| **snake_game** | [`apps/snake_game`](https://github.com/qmltui/apps/tree/main/apps/snake_game) | Subpixel `Canvas.HalfBlock` arcade game with speed levels, pause overlay, and WebAssembly support. |
| **twozero48** | [`apps/twozero48`](https://github.com/qmltui/apps/tree/main/apps/twozero48) | 2048 sliding-tile puzzle with `Behavior` color transitions, score tracking, and WebAssembly support. |
