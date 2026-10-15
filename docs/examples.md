# Examples & Component Cookbook

Build the example suite by configuring QmlTui with `-DQMLTUI_BUILD_EXAMPLES=ON`:

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

Enable `-DQMLTUI_BUILD_TESTS=ON` as well to register the automated `Runner` scenarios for each example. Full multi-screen application showcases are maintained in [qmltui-apps](https://github.com/qmltui-dev/qmltui-apps).

---

## Controls vs. Recipes Architecture

QmlTui draws a deliberate boundary between framework controls and application recipes:

- **`QmlTui.Controls` (Framework Primitives)**: Admits a type only when it requires engine knowledge that an application cannot cleanly reproduce in user QML (such as cell-grid geometry, Unicode grapheme width measurement, viewport virtualization, or overlay z-stacking) or when it is a universal primitive across terminal toolkits: `Button`, `CheckBox`, `Dialog`, `Panel`, `Popup`, `ProgressBar`, `RadioButton`, `Slider`, `Sparkline`, `StackLayout`, `StackView`, `StandardItemDelegate`, `Switch`, `TabBar`, `Toast`, and `ToolTip`.
- **Copyable Recipes (`examples/components/` & `examples/topics/`)**: Higher-level or domain-specific widgets—such as `Calendar`, `StatusBadge`, `BigText`, syntax-highlighted editors, and streaming coding-agent cards (`agent_stream`)—live as self-contained, BSD-3-Clause recipes under [`examples/`](../examples/).
- **Application-Owned Styling**: Core items and `QmlTui.Controls` default to the terminal's native foreground and background (`"default"`) and expose ordinary color properties whose explicit overrides survive focus and disabled state transitions. Because QmlTui does not impose a global theme singleton, you can copy a recipe directly into your project and customize its borders, colors, and bindings to match your application's visual language.

---

## 1. Interactive Gallery & Starter Projects

| Project | Source Directory | CMake Target | Description |
| :--- | :--- | :--- | :--- |
| **Interactive Component Gallery** | [`examples/gallery/`](../examples/gallery/) | `qmltui_gallery` | Browsable catalog of 30+ live component and layout tiles with keyboard and mouse navigation. |
| **Standalone Quickstart** | [`examples/quickstart/`](../examples/quickstart/) | `hello_qmltui` (`qmltui_quickstart_test` in-tree) | Minimal 3-file (`CMakeLists.txt`, `main.cpp`, `Main.qml`) standalone project using `FetchContent`. |
| **Six-Part Tutorial** | [`examples/tutorials/`](../examples/tutorials/) | `qmltui_tutorial_01_first_terminal` .. `06` | Progressive source code accompanying the [Six-Part Tutorial](tutorials/index.md). |
| **WebAssembly Playground** | [`examples/wasm/`](../examples/wasm/) | See [WebAssembly guide](topics/webassembly.md) | Browser-hosted interactive QmlTui playground sharing the gallery tiles. |

---

## 2. Reusable QML & C++ Recipes

Copy these self-contained components from [`examples/components/`](../examples/components/) or [`examples/topics/`](../examples/topics/) directly into your application module:

| Recipe | Source Files | CMake Demo Target | What It Provides |
| :--- | :--- | :--- | :--- |
| **StatusBadge** | [`examples/components/status_badge/StatusBadge.qml`](../examples/components/status_badge/) | `qmltui_status_badge_demo` | Compact status pill (`[OK]`, `[WARN]`, `[ERR]`) with configurable icon, label, border, and color presets. |
| **Calendar** | [`examples/components/calendar/Calendar.qml`](../examples/components/calendar/) | `qmltui_calendar_demo` | Month-grid date picker built from `Grid`, `Repeater`, keyboard navigation, and selection bindings. |
| **BigText** | [`examples/components/big_text/`](../examples/components/big_text/) (`BigText.h`, `BigText.cpp`) | `qmltui_big_text_demo` | Custom C++ `qmltui::Item` rendering multi-row block-pixel banner typography. |
| **Syntax Highlighting** | [`examples/components/text_area_highlighting/`](../examples/components/text_area_highlighting/) | `qmltui_text_area_highlighting_demo` | Both a QML-based highlighter (`QmlHighlighter.qml`) and a C++ JSON highlighter (`JsonHighlighter`) attached to `TextArea`. |
| **Streaming Agent UI** | [`examples/topics/agent_stream/`](../examples/topics/agent_stream/) | `qmltui_agent_stream_demo` | Complete inline coding-agent recipe (`Terminal.Stream` + `Scrollback`) with prompt bar, tool-call/diff/permission cards, and modal pickers. |

---

## 3. Component Demos (`examples/components/`)

Each directory under [`examples/components/`](../examples/components/) builds a focused executable (`cmake --build build --target <Target>`) and includes `Runner` verification scripts:

| Component | Directory | CMake Target | QML Reference | Key Concepts Demonstrated |
| :--- | :--- | :--- | :--- | :--- |
| **AnimatedSprite** | [`animated_sprite/`](../examples/components/animated_sprite/) | `qmltui_animated_sprite_demo` | [AnimatedSprite](qml-types/animatedsprite.md) | Frame-based sprite sheets, playback control (`QmlTui.Image`). |
| **BigText** | [`big_text/`](../examples/components/big_text/) | `qmltui_big_text_demo` | [Custom C++ Items](topics/custom-items.md) | Custom C++ `Item` recipe drawing multi-cell block characters. |
| **BusyIndicator** | [`busy_indicator/`](../examples/components/busy_indicator/) | `qmltui_busy_indicator_demo` | [BusyIndicator](qml-types/busyindicator.md) | Animated Braille/spinner styles, `running` state, custom colors. |
| **Button** | [`button/`](../examples/components/button/) | `qmltui_button_demo` | [Button](qml-types/button.md) | Focusable action buttons, double-border active focus, custom `background`/`contentItem`. |
| **Calendar** | [`calendar/`](../examples/components/calendar/) | `qmltui_calendar_demo` | Recipe | Reusable QML calendar grid with keyboard arrow navigation. |
| **Canvas** | [`canvas/`](../examples/components/canvas/) | `qmltui_canvas_demo` | [Canvas](qml-types/canvas.md) | Sub-cell Braille (2×4), sextant (2×3), half-block (1×2), and Kitty/Sixel pixel drawing. |
| **CheckBox** | [`check_box/`](../examples/components/check_box/) | `qmltui_check_box_demo` | [CheckBox](qml-types/checkbox.md) | Boolean and tristate checkboxes, keyboard Space/Enter toggling. |
| **Dialog** | [`dialog/`](../examples/components/dialog/) | `qmltui_dialog_demo` | [Dialog](qml-types/dialog.md) | Modal confirmation dialogs, `accepted`/`rejected` signals, Escape consumption. |
| **GridView** | [`grid_view/`](../examples/components/grid_view/) | `qmltui_grid_view_demo` | [GridView](qml-types/gridview.md) | Virtualized 2D tile grid with `cellWidth`/`cellHeight` and keyboard navigation. |
| **Image** | [`image/`](../examples/components/image/) | `qmltui_image_demo` | [Image](qml-types/image.md) | Terminal image rendering via Kitty graphics, Sixel, or Unicode half-blocks (`QmlTui.Image`). |
| **ListView** | [`list_view/`](../examples/components/list_view/) | `qmltui_list_view_demo` | [ListView](qml-types/listview.md) | Virtualized single-column list, `ListView.isCurrentItem`, scrolling and selection. |
| **Loader** | [`loader/`](../examples/components/loader/) | `qmltui_loader_demo` | [Loader](qml-types/loader.md) | Dynamic component instantiation via `sourceComponent` and `active`. |
| **LogView** | [`log_view/`](../examples/components/log_view/) | `qmltui_log_view_demo` | [LogView](qml-types/logview.md) | Append-optimized streaming log viewer with auto-follow and filtering. |
| **Panel** | [`panel/`](../examples/components/panel/) | `qmltui_panel_demo` | [Panel](qml-types/panel.md) | Bordered container with `title`, `titleBadge`, `titleTrailing`, and `footer` slots. |
| **Popup** | [`popup/`](../examples/components/popup/) | `qmltui_popup_demo` | [Popup](qml-types/popup.md) | Overlay popup with focus trapping, backdrop dimming, and Escape dismissal. |
| **ProgressBar** | [`progress_bar/`](../examples/components/progress_bar/) | `qmltui_progress_bar_demo` | [ProgressBar](qml-types/progressbar.md) | Horizontal progress bars with glyph styles, brackets, percentage, and `Gradient`/`thresholds`. |
| **RadioButton** | [`radio_button/`](../examples/components/radio_button/) | `qmltui_radio_button_demo` | [RadioButton](qml-types/radiobutton.md) | Mutually exclusive option buttons coordinated with `ButtonGroup`. |
| **Rectangle** | [`rectangle/`](../examples/components/rectangle/) | `qmltui_rectangle_demo` | [Rectangle](qml-types/rectangle.md) | Background fills, `Border` styles (`Single`, `Rounded`, `Double`, `Heavy`, `Ascii`), and `Gradient`. |
| **Repeater** | [`repeater/`](../examples/components/repeater/) | `qmltui_repeater_demo` | [Repeater](qml-types/repeater.md) | Instantiating positioner children from a model or integer count. |
| **ScrollView** | [`scroll_view/`](../examples/components/scroll_view/) | `qmltui_scroll_view_demo` | [ScrollView](qml-types/scrollview.md) | Clipped scrollable viewport with scrollbars and keyboard/wheel panning. |
| **Shortcut** | [`shortcut/`](../examples/components/shortcut/) | `qmltui_shortcut_demo` | [Shortcut](qml-types/shortcut.md) | Focus-independent key sequences (`Ctrl+S`, `F5`, etc.). |
| **Slider** | [`slider/`](../examples/components/slider/) | `qmltui_slider_demo` | [Slider](qml-types/slider.md) | Interactive range control adjusted via Left/Right arrows, Home/End, or mouse drag. |
| **Sparkline** | [`sparkline/`](../examples/components/sparkline/) | `qmltui_sparkline_demo` | [Sparkline](qml-types/sparkline.md) | Compact inline Unicode bar/trend chart (`▁▂▃▄▅▆▇█`) with `Gradient` coloring. |
| **StackView** | [`stack_view/`](../examples/components/stack_view/) | `qmltui_stack_view_demo` | [StackView](qml-types/stackview.md) | Push/pop/replace navigation stack for multi-screen workflows. |
| **StandardItemDelegate** | [`standard_item_delegate/`](../examples/components/standard_item_delegate/) | `qmltui_standard_item_delegate_demo` | [StandardItemDelegate](qml-types/standarditemdelegate.md) | Ready-made view delegate with selection highlighting (`qmltui_standard_item_delegate_widgets` shows embedded controls). |
| **StatusBadge** | [`status_badge/`](../examples/components/status_badge/) | `qmltui_status_badge_demo` | Recipe | Reusable status badge QML component. |
| **Switch** | [`switch/`](../examples/components/switch/) | `qmltui_switch_demo` | [Switch](qml-types/switch.md) | On/Off toggle control with keyboard and mouse activation. |
| **TabBar** | [`tab_bar/`](../examples/components/tab_bar/) | `qmltui_tab_bar_demo` | [TabBar](qml-types/tabbar.md) | Horizontal tab strip paired with `StackLayout` for tabbed workspaces. |
| **TableView** | [`table_view/`](../examples/components/table_view/) | `qmltui_table_view_demo` | [TableView](qml-types/tableview.md) | Virtualized multi-column table with headers, grid lines, sorting, and `QAbstractTableModel`. |
| **TerminalView** | [`terminal_view/`](../examples/components/terminal_view/) | `qmltui_terminal_view_demo` | [TerminalView](qml-types/terminalview.md) | Embedded Unix PTY shells in a split layout (`QMLTUI_ENABLE_TERMINAL_HOST=ON`). |
| **Text** | [`text/`](../examples/components/text/) | `qmltui_text_demo` | [Text](qml-types/text.md) | Word wrapping, elision, alignment, rich-text `spans`, and OSC 8 hyperlinks. |
| **TextArea** | [`text_area/`](../examples/components/text_area/) | `qmltui_text_area_demo` | [TextArea](qml-types/textarea.md) | Multi-line text editor with line numbers, selection, copy/paste, and scrolling. |
| **TextArea Highlighting** | [`text_area_highlighting/`](../examples/components/text_area_highlighting/) | `qmltui_text_area_highlighting_demo` | [TextArea](qml-types/textarea.md) | Custom syntax highlighting in QML and C++ on a `TextArea`. |
| **TextInput** | [`text_input/`](../examples/components/text_input/) | `qmltui_text_input_demo` | [TextInput](qml-types/textinput.md) | Single-line input field with cursor movement, `placeholderText`, password echo mode, and validation. |
| **Toast** | [`toast/`](../examples/components/toast/) | `qmltui_toast_demo` | [Toast](qml-types/toast.md) | Transient non-modal notification banner with auto-dismiss timer. |
| **ToolTip** | [`tooltip/` / `tool_tip/`](../examples/components/tool_tip/) | `qmltui_tool_tip_demo` | [ToolTip](qml-types/tooltip.md) | Contextual overlay hint attached to focused or hovered controls. |
| **TreeView** | [`tree_view/`](../examples/components/tree_view/) | `qmltui_tree_view_demo` | [TreeView](qml-types/treeview.md) | Virtualized hierarchical tree backed by `QAbstractItemModel` with expand/collapse navigation. |

---

## 4. Topic Demos (`examples/topics/`)

Each directory under [`examples/topics/`](../examples/topics/) illustrates an architectural pattern or subsystem in depth:

| Topic | Directory | CMake Target | Related Guide | What It Demonstrates |
| :--- | :--- | :--- | :--- | :--- |
| **Agent Stream** | [`agent_stream/`](../examples/topics/agent_stream/) | `qmltui_agent_stream_demo` | [Inline / Stream Mode](topics/inline-mode.md) | Interactive coding-agent session combining `Terminal.Stream`, `Scrollback`, prompt input, and modal dialogs. |
| **Anchors** | [`anchors/`](../examples/topics/anchors/) | `qmltui_anchors_demo` | [Layouts & Geometry](topics/layouts.md) | Relative cell positioning with `anchors.fill`, `centerIn`, `margins`, and edge attachment. |
| **Animation** | [`animation/`](../examples/topics/animation/) | `qmltui_animation_demo` | [Animation & States](topics/animation-and-states.md) | `NumberAnimation`, `ColorAnimation`, `SequentialAnimation`, and `ParallelAnimation` on cell properties. |
| **ANSI Styling** | [`ansi_styling/`](../examples/topics/ansi_styling/) | `qmltui_ansi_styling_demo` | [Styling & Colors](topics/styling-and-colors.md) | Text attributes (bold, italic, underline, dim, blink, reverse, strikethrough) and ANSI colors. |
| **Color Type Resolution** | [`color_type_resolution/`](../examples/topics/color_type_resolution/) | `qmltui_color_type_resolution_demo` | [Styling & Colors](topics/styling-and-colors.md) | How `"default"`, named ANSI colors, and `#RRGGBB` values quantize across TrueColor, 256-color, 16-color, and monochrome modes. |
| **C++-Only Application** | [`cpp_only/`](../examples/topics/cpp_only/) | `qmltui_cpp_only_demo` | [Custom C++ Items](topics/custom-items.md) | Constructing and updating a `qmltui::Terminal` and item tree purely from C++ without `.qml` files. |
| **Custom C++ Item** | [`custom_cpp_item/`](../examples/topics/custom_cpp_item/) | `qmltui_custom_cpp_item_demo` | [Custom C++ Items](topics/custom-items.md) | Subclassing `qmltui::Item`, overriding `paint(Painter *)`, and registering with `QML_ELEMENT`. |
| **Fluid Background** | [`fluid_background/`](../examples/topics/fluid_background/) | `qmltui_fluid_background_demo` | [Canvas](qml-types/canvas.md) | High-FPS procedural plasma/fluid shader rendered on a `Canvas` backdrop behind interactive controls. |
| **Focus & Keys** | [`focus_and_keys/`](../examples/topics/focus_and_keys/) | `qmltui_focus_and_keys_demo` | [Input & Focus](topics/input-and-focus.md) | `FocusScope`, `KeyNavigation`, Tab chains, and `Keys.onPressed` event propagation. |
| **Independent States** | [`independent_states/`](../examples/topics/independent_states/) | `qmltui_independent_states_demo` | [Animation & States](topics/animation-and-states.md) | Orthogonal state machines using `StateGroup` and `Behavior` (`QMLTUI_ENABLE_PRIVATE_QT=ON`). |
| **Inline REPL** | [`inline_repl/`](../examples/topics/inline_repl/) | `qmltui_inline_repl_demo` | [Inline / Stream Mode](topics/inline-mode.md) | Minimal command-line REPL using `Terminal.Stream` and `Scrollback.pushText()`. |
| **Layouts** | [`layouts/`](../examples/topics/layouts/) | `qmltui_layouts_demo` | [Layouts & Geometry](topics/layouts.md) | Comparing `Row`/`Column`/`Grid` positioners with `RowLayout`/`ColumnLayout`/`GridLayout` and `SplitView`. |
| **Lazy Model** | [`lazy_model/`](../examples/topics/lazy_model/) | `qmltui_lazy_model_demo` | [Models & Views](topics/models.md) | Virtualized C++ `QAbstractListModel` serving 1,000,000 rows on demand with zero upfront allocation. |
| **Live Image** | [`live_image/`](../examples/topics/live_image/) | `qmltui_live_image_demo` | [Architecture](topics/architecture.md) | Streaming dynamic `QImage` frames from C++ into a terminal `Image` item (`QMLTUI_ENABLE_IMAGE=ON`). |
| **Mouse Interaction** | [`mouse_interaction/`](../examples/topics/mouse_interaction/) | `qmltui_mouse_interaction_demo` | [Input & Focus](topics/input-and-focus.md) | `MouseArea` clicks, hover tracking, drag coordinates, and wheel scrolling via SGR mouse reporting. |
| **State Transitions** | [`state_transitions/`](../examples/topics/state_transitions/) | `qmltui_state_transitions_demo` | [Animation & States](topics/animation-and-states.md) | Declarative `State`, `PropertyChanges`, and animated `Transition` definitions. |
| **Terminal Capabilities** | [`terminal_capabilities/`](../examples/topics/terminal_capabilities/) | `qmltui_terminal_capabilities_demo` | [TerminalCapabilities](qml-types/terminalcapabilities.md) | Inspecting runtime terminal feature detection (`colorDepth`, `unicodeSupport`, Kitty protocols, synchronized output). |
