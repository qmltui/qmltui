# Input and focus

Set `focus: true` on the initial interactive item. Tab and Shift+Tab move
through focusable items. `Keys.onPressed` handles keyboard input; set
`event.accepted = true` when the handler consumes it. `Shortcut` handles
application commands independently of the focused item.

A root Terminal has no automatic quit key. Provide a reachable exit path,
usually Escape or Ctrl+Q. Let Escape close a modal, cancel an operation, or
clear input before quitting. Q typed into an editor must remain text.

Use `MouseArea` for clicks and hover. Terminal mouse reporting determines the
available motion events; keyboard navigation must remain usable independently.

See [focus and keys](../../examples/topics/focus_and_keys/),
[mouse interaction](../../examples/topics/mouse_interaction/),
[Keys](../qml-types/keys.md), and [Shortcut](../qml-types/shortcut.md).
