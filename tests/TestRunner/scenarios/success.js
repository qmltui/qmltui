function main(terminal) {
    terminal.waitForText("READY");
    terminal.waitForText("ASYNC READY");
    terminal.press("K");
    terminal.waitForText("KEY K");
    terminal.typeText("2");
    terminal.waitForText("KEY 2");
    terminal.typeText("+");
    terminal.waitForText("KEY PLUS");
    terminal.press("Tab");
    terminal.typeText("café 世界");
    terminal.press("a", ["Shift"]);
    terminal.waitForText("INPUT: café 世界A");
    terminal.expectNoText("type text here");
    terminal.captureText("success.txt");
    terminal.finish();
}
