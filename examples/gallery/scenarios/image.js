/** @param {Runner} app */
function main(app) {
    app.expectText("qrc:/gallery/assets/bike-picsum.jpg");
    app.expectText("Source 4912x3264 px");
    app.expectText("Auto: kitty or sixel where supported, else half-block");
    app.sleep(900);
    app.typeText("m");
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
