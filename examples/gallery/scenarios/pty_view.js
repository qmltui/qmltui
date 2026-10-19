/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("[RUNNING]");
    terminal.expectText("PID:");

    terminal.press("Enter");
    terminal.waitForText("Sent: echo 'Hello from PtyView!'");
    terminal.waitForText("Hello from PtyView!");
    terminal.captureText("pty_view.txt");
}
