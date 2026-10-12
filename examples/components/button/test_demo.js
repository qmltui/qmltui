/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Button & ButtonGroup Component Example");
    terminal.sleep(1500);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("Primary button clicked");
    terminal.sleep(1200);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("Secondary button clicked");
    terminal.sleep(1200);
    // Tab to ButtonGroup (Option A), then navigate to B and C
    terminal.press("Tab");
    terminal.sleep(600);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Space");
    terminal.expectText("Option B selected");
    terminal.sleep(1000);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Space");
    terminal.expectText("Option C selected");
    terminal.sleep(1500);
    terminal.stopRecording();
}
