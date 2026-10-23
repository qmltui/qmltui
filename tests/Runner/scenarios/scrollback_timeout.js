// Waiting for scrollback that never arrives fails with a bounded timeout.
function main(app) {
    app.expectText("INLINE FIXTURE");
    app.waitForScrollbackText("NEVER COMMITTED", 100);
}
