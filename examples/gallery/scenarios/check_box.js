/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("logging=true  verbose=false  colors=true");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Space");
    terminal.expectText("logging=true  verbose=true  colors=true");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Space");
    terminal.expectText("logging=true  verbose=true  colors=false");
    terminal.sleep(900);
    terminal.captureText("check_box.txt");
    terminal.stopRecording();
}
