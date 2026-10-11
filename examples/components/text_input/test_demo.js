/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TextInput Component Demo");
    terminal.sleep(1500);
    // Navigate through the four cursor-style fields
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(1000);
    // Type into the password field (no Q key — safe against quit handler)
    terminal.typeText("hunter2");
    terminal.sleep(1500);
    terminal.stopRecording();
}
