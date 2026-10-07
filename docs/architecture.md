# Architecture

Qtui provides a retained-mode layout and rendering tree for terminal applications, heavily inspired by Qt Quick and QML. It translates QML item trees into terminal ANSI escape codes and translates terminal input into standard Qt-like events.

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
1. **FrameRenderer**: Maintains a complete terminal cell frame in the alternate screen buffer (`\033[?1049h`), calculates differences between frames on a `TextGridBuffer`, and issues minimal ANSI escape sequences to paint changes dynamically without flicker.
2. **StreamRenderer**: Streams permanent output through the normal terminal buffer while redrawing only the live region at the bottom using relative cursor positioning.

`Terminal.mode` selects `FullScreen` or `Inline`; ordinary visible root children
form the UI in either mode. `Scrollback` submits output to a bounded session
queue independently of the active renderer. Inline commits combine pending
output and the live region in one write. Full-screen mode retains pending
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

## Agent testing feedback loop

The optional `Qtui::TestRunner` library attaches a `TestRunner` to an explicit
`Terminal`. Automated sessions suppress terminal-device output while retaining
normal frame commits, input dispatch, QML timers, and render-failure signals.
Each run writes an atomic schema-versioned result, a normalized final frame,
and requested captures beneath a contained artifact root. Live rendering is an
explicit presentation mode rather than the default test path.

## Input Handling

Terminal input happens asynchronously via standard input byte streams.
The `TerminalDevice` handles reading raw bytes and forwards them to the `TerminalEngine`.
`InputDecoder` processes these byte streams (often UTF-8 and ANSI escape sequences) to deduce mouse events, standard keypresses, and complex multi-key combinations. It translates these into `KeyEvent` and `MouseEvent` objects, which are routed to the focused item.

## Node Graph
The visual node graph consists of elements inheriting from `qtui::Item`. 
Each `Item` corresponds to a declarative block in QML. Items manage their relative x/y positions, implicit width/height, and arbitrary children. State changes mark the tree as "dirty", prompting the `TerminalEngine` to schedule an asynchronous re-render in the main Qt event loop.
