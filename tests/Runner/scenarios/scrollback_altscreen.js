// Scrollback assertions refuse an AltScreen terminal, where rows stay pending.
function main(app) {
    app.expectScrollbackText("anything");
}
