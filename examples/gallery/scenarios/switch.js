/** @param {Runner} app */
function main(app) {
    app.expectText("Wi-Fi on, Bluetooth off");
    app.sleep(800);
    app.press("Tab");
    app.sleep(500);
    app.press("Space");
    app.expectText("Wi-Fi on, Bluetooth on");
    app.sleep(800);
    app.press("Tab");
    app.sleep(500);
    app.press("Space");
    app.expectText("Radios are off while airplane mode is on");
    app.sleep(900);
    app.captureText("switch.txt");
    app.stopRecording();
}
