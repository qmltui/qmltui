/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Hello, stranger!");
    terminal.sleep(700);
    terminal.typeText("Ada");
    terminal.expectText("Hello, Ada!");
    terminal.sleep(700);
    terminal.press("Tab");
    terminal.typeText("s3cr3t");
    terminal.expectText("Password has 6 characters.");
    terminal.sleep(900);
    terminal.captureText("text_input.txt");
    terminal.stopRecording();
}
