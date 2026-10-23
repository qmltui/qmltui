// A resource baseline that never matches the fixture frame. Update mode
// cannot rewrite it and fails with the resource diagnostic; normal mode
// fails with an ordinary snapshot mismatch. The embedded API tests use the
// two messages to tell which mode setUpdateSnapshots() selected.
function main(app) {
    app.waitForText("STATUS: ASYNC READY");
    app.expectSnapshot("snapshots/stale_seed.txt");
}
