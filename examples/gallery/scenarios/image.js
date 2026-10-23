/** @param {Runner} app */
function main(app) {
    app.expectText("Braille: eight dots per cell");
    app.sleep(900);
    app.typeText("m");
    app.expectText("HalfBlock: two pixels per cell");
    app.sleep(900);
    app.typeText("m");
    app.expectText("ASCII: brightness as characters");
    app.sleep(900);
    app.captureText("image.txt");
    app.stopRecording();
}
