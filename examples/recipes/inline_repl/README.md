# Inline REPL

Interactive inline CLI prompt built with `Terminal.Inline`, `Scrollback`, `Canvas`, `ProgressBar`, and `TextInput`.

- **Presentation mode**: `mode: Terminal.Inline` keeps the session in the normal terminal buffer and sizes the live bottom viewport from its visible children (`Column` with status badge, `ProgressBar`, `TextInput` prompt, and keybinding hint).
- **Scrollback banner and output**: `Component.onCompleted` calls `history.append({})` on `Scrollback` to commit the multi-row startup banner (`Canvas` QmlTui mark + styled `Text` spans) above the live UI. Pressing `Return` in the `TextInput` calls `history.print({ text: "❯ " + text, color: "cyan" })` to commit each command into native terminal scrollback.
- **Quit path**: `Escape` clears `prompt.text` first when non-empty, and calls `Qt.quit()` when the prompt is already empty.

```bash
cmake --build build --target qmltui_inline_repl_demo
./build/examples/recipes/inline_repl/qmltui_inline_repl_demo
ctest --test-dir build -R '^inline_repl_smoke$' --output-on-failure
```
