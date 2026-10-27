/** @param {Runner} app */
function main(app) {
    app.expectText("This tip is pinned with ToolTip.visible");
    app.sleep(300);
    app.sleep(300);
    app.expectText("Compile the current project");
    app.sleep(600);
    app.press("Tab");
    app.sleep(300);
    app.sleep(300);
    app.expectText("Push the build to staging");
    app.sleep(700);
    app.captureText("tool_tip.txt");
    app.stopRecording();
}
