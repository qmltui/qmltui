/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Last link: none yet");
    terminal.sleep(800);
    terminal.clickText("QML tutorial");
    terminal.expectText("https://doc.qt.io/qt-6/qml-tutorial.html");
    terminal.sleep(900);
    terminal.captureText("rich_text.txt");
    terminal.stopRecording();
}
