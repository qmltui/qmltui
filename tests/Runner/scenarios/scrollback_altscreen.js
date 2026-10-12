// Scrollback assertions refuse an AltScreen terminal, where rows stay pending.
function main(terminal) {
    terminal.expectScrollbackText("anything");
}
