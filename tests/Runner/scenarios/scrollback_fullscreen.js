// Scrollback assertions refuse a FullScreen terminal, where rows stay pending.
function main(terminal) {
    terminal.expectScrollbackText("anything");
}
