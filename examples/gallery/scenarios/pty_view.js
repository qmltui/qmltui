/** @param {Runner} app */
function main(app) {
    app.waitForText("[RUNNING]");
    app.expectText("PID:");
    app.sleep(800);

    app.press("Enter");
    app.waitForText("Sent: echo 'Hello from PtyView!'");
    app.waitForText("Hello from PtyView!");
    app.sleep(1000);
    app.captureText("pty_view.txt");
    app.stopRecording();
}
