# C++ only

A filterable service list built from QmlTui's C++ items. No QML file and no QML
engine during normal startup: `QCoreApplication`, a `Terminal` subclass, and one `QProperty` binding.

```text
 SERVICE MONITOR  ·  C++ only, no QML engine

 Filter: type part of a service name

 auth-service             12 ms ● healthy
 database-primary          6 ms ● healthy
 worker-queue             85 ms ▲ degraded

 Esc clears the filter, then quits  ·  Ctrl+Q quits anywhere
```

- **Rows** are built by `addRow()`, the C++ counterpart of a delegate: a `Row`
  of three `Text` items parented to a `Column`. `ListView` and `Repeater`
  take a `QQmlComponent` delegate, so they need a QML engine and are not used.
- **State** is two written properties, `m_services` and `m_filterText`, and one
  binding, `m_visibleServices`. `QBindable<QString>(textInput, "text")` adapts
  the ordinary `Q_PROPERTY` so the binding can depend on it. A
  `QPropertyNotifier` rebuilds the rows when the binding changes.
- **Keys** reach the root `Terminal` only after the focused widget declined
  them. The filter accepts every printable character, so `q` is text; `Ctrl+Q`
  and `Escape` are handled in `keyPressEvent`. A `Shortcut` is matched *before*
  the focused item and ignores modifiers, so a `Shortcut` on `Qt::Key_Q` would
  steal the letter from the text field.

```bash
cmake --build build --target qmltui_cpp_only_demo
./build/examples/topics/cpp_only/qmltui_cpp_only_demo
ctest --test-dir build -R '^cpp_only_dashboard$' --output-on-failure
```

When Runner is available, the same executable supports environment-driven
scenarios. `main.cpp` creates a script engine and calls `startFromEnvironment()`
only when a Runner session is requested, because the automatic hook requires a
QML-created Terminal. The scenario drives telemetry through
`terminal.rootObject().tick()`.
