# Terminal Capabilities

A QmlTui topic example inspecting and printing host terminal capabilities.

Build with `cmake --build build --target qmltui_terminal_capabilities_demo`.
Run `./build/examples/topics/terminal_capabilities/qmltui_terminal_capabilities_demo`.
See [main.cpp](main.cpp) for startup and [Main.qml](Main.qml) for the interface.

The example demonstrates:
- Querying host terminal capabilities via `Terminal.capabilities` (`terminalCapabilities` value type).
- Inspecting `syncOutput`, `kittyKeyboard`, `kittyGraphics`, `sixelGraphics`, `cellPixelSize`, `colorDepth`, `termProgram`, and `isTty`.
- Printing capability summaries to `Scrollback`.

Run its scenario with `ctest --test-dir build -R terminal_capabilities --output-on-failure`.
