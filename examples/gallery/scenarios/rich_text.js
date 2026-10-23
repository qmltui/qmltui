/** @param {Runner} app */
function main(app) {
    app.expectText("Last link: none yet");
    app.sleep(800);
    app.clickText("QML tutorial");
    app.expectText("https://doc.qt.io/qt-6/qml-tutorial.html");
    app.sleep(900);
    app.captureText("rich_text.txt");
    app.stopRecording();
}
