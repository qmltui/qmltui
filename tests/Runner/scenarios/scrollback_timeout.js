// Waiting for scrollback that never arrives fails with a bounded timeout.
function main(terminal) {
    terminal.expectText("INLINE FIXTURE");
    terminal.waitForScrollbackText("NEVER COMMITTED", 100);
}
