/** @param {Runner} app */
function main(app) {
    app.expectText("BigText — 3x3 Block Glyph Item");
    app.advanceTime(200);
    app.captureText("big_text.txt");
}
