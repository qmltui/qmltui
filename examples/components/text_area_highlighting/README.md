# TextArea Highlighting Component Example

Demonstrates range-based highlighting with `TextArea` in Qamui:
- Layered highlight management: independent `syntax` (priority 0) and `search` (priority 10) layers.
- Cascaded style resolution: search background highlighting overlays syntax foreground coloring without overwriting token foregrounds.
- Dual highlighter architectures:
  - `JsonHighlighter` (C++): tokenizes JSON text with comments (`//` and `/* ... */`), caches tokens, and drives `setHighlightLayer()`.
  - `QmlHighlighter` (pure QML): demonstrates that syntax and search highlighting can be implemented entirely in QML/JavaScript without any C++ backend.
- Document revision tracking: highlighters synchronize via `editor.textRevision`, ensuring submissions match the current document state and stale ranges from previous edits are rejected.
- Keyboard shortcuts:
  - `F2` (or `Ctrl+T`): Toggles active color palette / theme at runtime.
  - `F3` (or `Ctrl+M`): Switches between JSON document (C++ highlighter) and QML document (pure QML highlighter).
  - `Escape`: Exits the application cleanly; typing in the editor remains unrestricted.
