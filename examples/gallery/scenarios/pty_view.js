/** @param {Runner} app */
function main(app) {
    app.waitForText("[RUNNING]");
    app.expectText("PID:");

    app.press("Enter");
    app.waitForText("Sent: echo 'Hello from PtyView!'");
    app.waitForText("Hello from PtyView!");
    app.captureText("pty_view.txt");
}
