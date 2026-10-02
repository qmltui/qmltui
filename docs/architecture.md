# Architecture

Qtui provides a retained-mode layout and rendering tree for terminal applications, heavily inspired by Qt Quick and QML. It translates QML item trees into terminal ANSI escape codes and translates terminal input into standard Qt-like events.

## Core Loop
The execution is driven by standard Qt event loops (usually `QCoreApplication`). The `TerminalEngine` is responsible for bridging the gap between the event loop and the terminal.
When the user instantiates a `Terminal` object as the root of their QML file, the `TerminalEngine` is created. It connects the QML rendering lifecycle to terminal I/O.

## Rendering Pipeline
The rendering model is "retained mode", meaning the component tree stays resident in memory and handles its own layout and drawing properties.
The `Renderer` interface dictates how the application is drawn to the terminal.
There are two primary renderers:
1. **FrameRenderer**: Maintains a complete terminal cell frame. It can use the alternate screen buffer (`\033[?1049h`), calculates differences between frames using a painter's algorithm on a `TextGridBuffer`, and issues minimal ANSI escape sequences (`\033[%d;%dH`) to paint changes dynamically without flicker.
2. **StreamRenderer**: Streams permanent output through the normal terminal buffer while redrawing only the live region at the bottom using relative cursor positioning.

Every frame, items traverse the tree, computing bounds, clip rectangles, and painting `Cell` structures containing characters, colors, and ANSI attributes to a `TextGridBuffer`. 

## Input Handling
Terminal input happens asynchronously via standard input byte streams.
The `TerminalDevice` handles reading raw bytes and forwards them to the `TerminalEngine`.
`InputDecoder` processes these byte streams (often UTF-8 and ANSI escape sequences) to deduce mouse events, standard keypresses, and complex multi-key combinations. It translates these into `KeyEvent` and `MouseEvent` objects, which are routed to the focused item.

## Node Graph
The visual node graph consists of elements inheriting from `qtui::Item`. 
Each `Item` corresponds to a declarative block in QML. Items manage their relative x/y positions, implicit width/height, and arbitrary children. State changes mark the tree as "dirty", prompting the `TerminalEngine` to schedule an asynchronous re-render in the main Qt event loop.
