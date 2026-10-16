# Architecture

QmlTui provides a retained-mode layout and rendering tree for terminal applications, heavily inspired by Qt Quick and QML. It translates QML item trees into terminal ANSI escape codes and translates terminal input into standard Qt-like events. For the rationale behind retained mode and property bindings versus immediate-mode terminal frameworks, see [why declarative bindings](why-declarative-bindings.md).

## Core Loop
The execution is driven by standard Qt event loops (usually `QCoreApplication`). The `TerminalEngine` is responsible for bridging the gap between the event loop and the terminal.
The root `Terminal` initializes its engine after QML completion, when parentage
and mode are available. C++ applications can initialize it through `engine()`;
otherwise it initializes on the next event-loop turn. The engine connects the
rendering lifecycle to terminal I/O and survives presentation-mode changes.

## Rendering Pipeline
The rendering model is "retained mode", meaning the component tree stays resident in memory and handles its own layout and drawing properties.
The `Renderer` interface dictates how the application is drawn to the terminal.
There are two primary renderers:
1. **AltScreenRenderer**: Maintains a complete terminal cell frame in the alternate screen buffer (`\033[?1049h`), calculates differences between frames on a `TextGridBuffer`, and issues minimal ANSI escape sequences to paint changes dynamically without flicker.
2. **InlineRenderer**: Streams permanent output through the normal terminal buffer while redrawing only the live region at the bottom using relative cursor positioning.

`Terminal.mode` selects `AltScreen` or `Inline`; ordinary visible root children
form the UI in either mode. `Scrollback` submits output to a bounded session
queue independently of the active renderer. Inline commits combine pending
output and the live region in one write. AltScreen mode retains pending
output until inline resumes or shutdown restores the normal screen.

Every frame, items traverse the tree, computing bounds, clip rectangles, and painting fixed-size `Cell` values (inline UTF-8 cluster bytes, colors, and ANSI attributes) into a `TextGridBuffer`.

## Terminal-safe Qt diagnostics

`TerminalEngine` registers a process-wide Qt message router while terminal
engines exist. Messages are forwarded normally until a renderer enters the
alternate screen. While any alternate-screen session is active, non-fatal Qt
messages are buffered so stderr cannot disturb ANSI cursor positioning. The
last session restores the terminal before replaying the messages through the
handler that was installed by the application. Engine and session counts make
this safe for multiple simultaneous `TerminalEngine` instances.

`TerminalDevice` remains limited to terminal byte I/O; it does not own Qt
logging policy. Fatal Qt messages are forwarded immediately because Qt aborts
after their handler returns.

## Input Handling

Terminal input happens asynchronously via standard input byte streams.
The `TerminalDevice` handles reading raw bytes and forwards them to the `TerminalEngine`.
`InputDecoder` processes these byte streams (often UTF-8 and ANSI escape sequences) to deduce mouse events, standard keypresses, and complex multi-key combinations. It translates these into `KeyEvent` and `MouseEvent` objects, which are routed to the focused item.

## Node Graph
The visual node graph consists of elements inheriting from `qmltui::Item`.
Each `Item` corresponds to a declarative block in QML. Items manage their relative x/y positions, implicit width/height, and arbitrary children. State changes mark the tree as "dirty", prompting the `TerminalEngine` to schedule an asynchronous re-render in the main Qt event loop.

## Module Boundaries & What `QmlTui.Core` Provides Alone

Linked as `qmltui::core` in CMake and imported as `import QmlTui` in QML, **`QmlTui.Core` is a complete, standalone retained-mode terminal UI engine**. It depends only on `Qt6::Core`, `Qt6::Qml`, and `utf8proc`—with zero dependency on `QtQuick`, `Qt6::Gui`, `Qt6::Network`, or a display server.

Even without linking higher-level modules, `QmlTui.Core` provides all the primitives needed to build full-featured terminal applications:

### 1. Capabilities Included in `QmlTui.Core` (`import QmlTui`)

- **Terminal Lifecycle & Presentation Modes**:
  - [`Terminal`](../qml-types/terminal.md): Root application item supporting [`Terminal.AltScreen`](../qml-types/terminal.md) (`AltScreenRenderer` with differential cell updates on the `\033[?1049h` alternate buffer) and [`Terminal.Inline`](../topics/inline-mode.md) (`InlineRenderer` for live bottom viewports inside normal shell scrollback).
  - [`Scrollback`](../qml-types/scrollback.md): Streams permanent output into terminal history without disrupting live inline UI regions.
  - [`terminalCapabilities`](../qml-types/terminalcapabilities.md) & [`QtMessageRouting`](../cpp-classes/qtmessagerouting.md): Runtime inspection of color depth/protocol support and automatic buffering of Qt diagnostic messages while alternate-screen sessions are active.
- **Visual & Styling Primitives**:
  - [`Item`](../qml-types/item.md) and [`Rectangle`](../qml-types/rectangle.md): Integer cell geometry, clipping, opacity, background colors (`Color` with automatic TrueColor / ANSI 256 / ANSI 16 quantization), [`Gradient`](../qml-types/gradient.md) / [`GradientStop`](../qml-types/gradientstop.md), and Unicode box-drawing [`Border`](../qml-types/border.md) styles (`Single`, `Double`, `Rounded`, `Heavy`, `Ascii`).
  - [`Text`](../qml-types/text.md): Word wrapping, alignment, ANSI attributes (`bold`, `dim`, `italic`, `underline`, `strikethrough`, `blink`, `inverse`), inline [`span`](../qml-types/span.md) formatting, [`highlightRange`](../qml-types/highlightrange.md), and Unicode grapheme width measurement via `utf8proc`.
- **Layouts & Dynamic Instantiation**:
  - Anchors: [`Anchors`](../qml-types/anchors.md) (`anchors.fill`, `anchors.centerIn`, edge anchors, margins) and [`AnchorLine`](../qml-types/anchorline.md).
  - Positioners & Layout Engines: [`Row`](../qml-types/row.md), [`Column`](../qml-types/column.md), [`RowLayout`](../qml-types/rowlayout.md), [`ColumnLayout`](../qml-types/columnlayout.md), [`GridLayout`](../qml-types/gridlayout.md), [`StackLayout`](../qml-types/stacklayout.md), and attached [`Layout`](../qml-types/layout.md) sizing properties.
  - Dynamic Instantiation: [`Repeater`](../qml-types/repeater.md) and deferred/conditional [`Loader`](../qml-types/loader.md).
- **Interactive Text Editing & Clipboard**:
  - [`TextInput`](../qml-types/textinput.md): Single-line editor with cursor navigation, selection, input masks, validators, and password echo modes.
  - [`TextArea`](../qml-types/textarea.md): Multi-line text editor with scrolling, line numbers, cursor styling, tab handling, and programmatic syntax highlight layers/ranges.
  - [`Clipboard`](../qml-types/clipboard.md): Terminal clipboard integration (OSC 52 and platform clipboard).
- **Virtualized Data Views (Model-View-Delegate)**:
  - [`ListView`](../qml-types/listview.md), [`TableView`](../qml-types/tableview.md), [`TreeView`](../qml-types/treeview.md), [`GridView`](../qml-types/gridview.md), and [`ScrollView`](../qml-types/scrollview.md), backed by C++ `QAbstractItemModel` / `QAbstractTableModel` subclasses, `ListModel`, or JavaScript arrays.
- **Vector & Subpixel Terminal Graphics ([`Canvas`](../qml-types/canvas.md))**:
  - Direct terminal drawing using 2×4 Braille subpixel matrices and block glyphs via [`CanvasShape`](../qml-types/canvasshape.md), [`CanvasRectangle`](../qml-types/canvasrectangle.md), [`Line`](../qml-types/line.md), [`Circle`](../qml-types/circle.md), [`Point`](../qml-types/point.md), and [`Polygon`](../qml-types/polygon.md).
- **Keyboard & Mouse Input Routing**:
  - Attached [`Keys`](../qml-types/keys.md) handlers (`onPressed`, `onReleased`), [`KeyEvent`](../qml-types/keyevent.md), [`Shortcut`](../qml-types/shortcut.md), [`MouseArea`](../qml-types/mousearea.md), [`MouseEvent`](../qml-types/mouseevent.md) (SGR 1006 / X10 mouse tracking, hover, wheel, drag), and `FocusManager` tab chains.
- **Animations & State Machines**:
  - [`Animation`](../qml-types/animation.md), [`PropertyAnimation`](../qml-types/propertyanimation.md), [`NumberAnimation`](../qml-types/numberanimation.md), [`ColorAnimation`](../qml-types/coloranimation.md), [`Easing`](../qml-types/easing.md), [`State`](../qml-types/state.md), [`PropertyChanges`](../qml-types/propertychanges.md), and [`Transition`](../qml-types/transition.md) (plus [`Behavior`](../qml-types/behavior.md) and [`StateGroup`](../qml-types/stategroup.md) when built with `-DQMLTUI_ENABLE_PRIVATE_QT=ON`).
- **C++ Extensibility**:
  - Subclass [`qmltui::Item`](../cpp-classes/item.md) (or `qmltui::Popup` for modal/overlay scopes) to paint directly onto [`TextGridBuffer`](../cpp-classes/textgridbuffer.md) with [`Cell`](../cpp-classes/cell.md) and [`Color`](../cpp-classes/color.md), or supply a custom [`TerminalBackend`](../cpp-classes/terminalbackend.md).

### 2. Companion QML Modules & Admission Boundaries

Everything outside `QmlTui.Core` is layered as an optional or higher-level QML module:

| Module / Target | QML Import | Responsibility & Boundary |
| :--- | :--- | :--- |
| **`QmlTui.Controls`** (`qmltui::controls`) | `import QmlTui.Controls` | Ready-made controls (`Button`, `AbstractButton`, `ButtonGroup`, `CheckBox`, `RadioButton`, `Switch`, `Slider`, `ProgressBar`, `BusyIndicator`, `Sparkline`, `TabBar`, `StackView`, `Panel`, `Dialog`, `Popup`, `Toast`, `ToolTip`). Admits a type only if it needs engine knowledge (`Popup` overlay stacking, cell geometry) or is universal across terminal toolkits; domain-specific widgets live as copyable recipes under `examples/components/`. |
| **`QmlTui.Image`** (`qmltui::image`) | `import QmlTui.Image` | Pixel-to-terminal image and sprite rendering (`Image`, `AnimatedSprite` via Kitty graphics or half-block cells). Requires `Qt6::Gui` and `Qt6::Network` (`-DQMLTUI_ENABLE_IMAGE=ON`). |
| **`QmlTui.TerminalHost`** (`qmltui::terminal_host`) | `import QmlTui.TerminalHost` | Embedded Unix PTY terminal widget (`TerminalView`) backed by `libvterm` on Linux and macOS (`-DQMLTUI_ENABLE_TERMINAL_HOST=ON`). |

### 3. Minimal Standalone `QmlTui.Core` Example

Because `QmlTui.Core` includes `Rectangle`, `Text`, `TextInput`, `MouseArea`, `Keys`, and layouts, an application linking only `qmltui::core` can compose its own interactive controls directly in QML without importing `QmlTui.Controls`:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root
    color: "#0f141c"

    component CoreButton: Rectangle {
        id: button
        property string label: "Submit"
        signal clicked()

        implicitWidth: labelItem.implicitWidth + 4
        implicitHeight: 3
        focus: true
        color: activeFocus ? "#1f3b5c" : (mouseArea.containsMouse ? "#162a42" : "#121d2b")
        border {
            color: activeFocus ? "cyan" : "gray"
            style: Border.Rounded
        }

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                button.clicked();
                event.accepted = true;
            }
        }

        Text {
            id: labelItem
            anchors.centerIn: parent
            text: button.label
            color: button.activeFocus ? "white" : "#cbd5e1"
            bold: button.activeFocus
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                button.forceActiveFocus();
                button.clicked();
            }
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Built with QmlTui.Core alone"
            color: "cyan"
            bold: true
        }

        TextInput {
            id: input
            width: 30
            placeholderText: "Type something..."
            focus: true
        }

        CoreButton {
            label: "Submit"
            onClicked: statusText.text = "You entered: " + (input.text || "(empty)")
        }

        Text {
            id: statusText
            text: "Ready"
            color: "yellow"
        }
    }
}
```
