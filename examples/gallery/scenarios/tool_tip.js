/** @param {Runner} app */
function main(app) {
    app.expectText("This tip is pinned with ToolTip.visible");
    app.sleep(500);
    app.expectText("Compile the current project");
    app.sleep(800);
    app.press("Tab");
    app.sleep(500);
    app.expectText("Push the build to staging");
    app.sleep(900);
    app.captureText("tool_tip.txt");
    app.stopRecording();
}
