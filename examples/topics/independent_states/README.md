# Independent States and Transitions Demo

Demonstrates independent focus and connection states in a keyboard-driven terminal
interface, showcasing `StateGroup`, `Item.states`, direct `PropertyChanges` assignments,
and predictable transition interpolation.

## Features Illustrated

- **Independent State Controllers**: `StateGroup` manages system connection lifecycle
  (`disconnected`, `connecting`, `connected`) independently from the `Item.states` managing
  keyboard focus on individual node cards.
- **Direct Assignment Syntax**: Uses both `target: card; border.color: ...` and ID-qualified
  property assignments `card1Header.color: ...`.
- **Precedence and No-Overlap Isolation**: Each controller operates on its own properties
  without colliding with other controllers.
- **Contextual Quit Path**: `Escape` contextually cancels an in-flight connection before
  reverting to standard exit behavior.
- **Virtual-Clock Validation**: Verified using QmlTui Runner with deterministic time advancement
  across entry, midpoint, completion, interruption, resize, and restoration phases.

## Running

```bash
# Direct run
./build/examples/topics/independent_states/qmltui_independent_states_demo

# Runner scenario test
ctest --test-dir build -R independent_states_scenario --output-on-failure
```

## Keyboard Controls

| Key | Action |
| :--- | :--- |
| `Tab` / `Shift+Tab` | Toggle focus between Primary and Secondary Node cards |
| `C` | Connect or disconnect cluster session |
| `R` | Reset back to base disconnected state |
| `Escape` | Contextually cancel active connection, or quit if idle |
| `Q` | Quit application |
