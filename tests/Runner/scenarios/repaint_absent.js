// The counterpart of repaint_scheduled.js: once the fixture's one-shot timer
// has fired and its frame is committed, nothing is changing, so expectRepaint()
// must fail rather than silently succeed on a frame it requested itself.
function main(app) {
    app.waitForText("STATUS: ASYNC READY", 2000);
    app.expectRepaint(400);
}
