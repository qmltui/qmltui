/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("RowLayout");
    terminal.expectText("GridLayout");
    terminal.expectText("0,2 span 2");
    terminal.sleep(1200);
    terminal.captureText("layouts.txt");
    terminal.stopRecording();
}
