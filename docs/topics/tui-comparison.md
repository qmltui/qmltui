# Qamui for Terminal UI Developers

If you are coming from immediate-mode TUI frameworks such as **Ratatui** (Rust),
**Bubble Tea** (Go), **Textual** (Python), or **Ink** (React/JavaScript), Qamui
offers a fundamentally different architectural model.

While those frameworks typically rely on procedural render loops and state diffing,
Qamui brings Qt's mature **retained-mode declarative engine** and C++20 performance
to terminal screens.

---

## 1. Retained Mode vs Immediate Mode

In an immediate-mode TUI (like Ratatui), your code runs an event loop that
re-describes the entire layout tree on every tick:

```rust
// Typical immediate-mode loop (Ratatui)
loop {
    let event = poll_event();
    update_model(&mut app, event);
    terminal.draw(|f| {
        // Entire UI is recalculated and rebuilt every frame
        render_dashboard(f, &app);
    })?;
}
```

In Qamui, you declare the view hierarchy and property relationships **once**.
The Qt event loop and QML engine manage state updates automatically:

```qml
// Retained-mode declarative view (Qamui)
Terminal {
    Text {
        // Automatically updates ONLY when server.status changes
        text: "Server: " + server.status
        color: server.isOnline ? "green" : "red"
    }
}
```

When `server.status` changes, only that single `Text` element re-evaluates.
Qamui marks only the dirty cells in the double-buffered text grid and flushes
the minimal ANSI diff to the terminal.

---

## 2. Model/View Virtualization

In immediate-mode frameworks, rendering large collections (e.g., 50,000 log entries
or processes) requires the application to manually manage scroll offsets and slice
arrays:
```rust
// Manual slice calculation
let visible_items = &logs[scroll_offset..scroll_offset + screen_height];
```

In Qamui, standard Qt models (`QAbstractListModel`, `QAbstractTableModel`) handle
this out of the box:
- `ListView`, `TableView`, and `TreeView` instantiate only the visible delegates.
- As the user scrolls via arrow keys or mouse wheel, delegates are reused and
  populated via Qt model index queries.
- Model modifications (insertions, removals, sorting) emit Qt signals that notify
  the view to update with zero manual offset math.

---

## 3. Asynchronous Data & Threading

In CLI tools, terminal UIs often monitor background jobs, parse network streams,
or control hardware.

In many TUI frameworks, sharing state between background tasks and the render loop
requires explicit channel passing (e.g. `mpsc::channel` in Rust or `tea.Msg` in Go).

In Qamui:
- **C++ Backend**: Background threads emit standard Qt signals.
- **Thread Safety**: Qt automatically routes cross-thread signal connections
  (`Qt::QueuedConnection`) into the main thread's event loop.
- **Declarative QML**: The UI binds directly to properties exposed by your C++
  backend with full thread safety:

```cpp
// In your C++ worker:
emit temperatureChanged(newTemp); // Safe from any background thread
```

```qml
// In Qamui QML:
Sparkline {
    data: telemetryWorker.history
}
```

---

## 4. Architectural Comparison Table

| Feature | Ratatui (Rust) / Bubble Tea (Go) | Qamui (C++20 / QML) |
| :--- | :--- | :--- |
| **UI Paradigm** | Immediate mode (re-render every tick) | Retained mode (declarative reactive bindings) |
| **State Sync** | Application recalculates on every tick | Fine-grained dependency tracking per property |
| **Layout System** | Procedural Rect splits (`Layout::default()`) | Declarative Anchors, Row, Column, Grid |
| **Virtualization** | Manual array slice calculation | Built-in via `QAbstractItemModel` & `ListView` |
| **Animations** | Manual tick interpolation | Retained `NumberAnimation`, `PropertyAnimation` |
| **Testing** | Headless buffer assertions | Built-in [`Qamui::Runner`](../cpp-classes/runner.md) frame snapshot testing |
| **Web Browser** | Custom WASM wrappers | Built-in Qt for WebAssembly via `xterm.js` |
