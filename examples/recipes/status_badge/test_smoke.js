/** @param {Runner} app */
function main(app) {
    app.expectText("QmlTui StatusBadge Component Demo");
    app.advanceTime(200);
    app.captureText("status_badge.txt");
}
