/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("This tip is pinned with ToolTip.visible");
    terminal.sleep(500);
    terminal.expectText("Compile the current project");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.expectText("Push the build to staging");
    terminal.sleep(900);
    terminal.captureText("tool_tip.txt");
    terminal.stopRecording();
}
