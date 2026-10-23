/** @param {Runner} app */
function main(app) {
    app.expectText("BigText — 3x5 Block Glyph Item");
    app.sleep(3000);
    app.stopRecording();
}
