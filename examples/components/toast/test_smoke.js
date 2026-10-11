/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TOASTMANAGER NOTIFICATIONS DEMO");
    terminal.advanceTime(200);
    terminal.captureText("toast.txt");
}
