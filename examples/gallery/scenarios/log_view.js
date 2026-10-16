/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("LogView · Streaming Log Analyzer");
    terminal.expectText("[INFO]  engine: terminal session ready");
    terminal.expectText("[WARN]  cache: glyph table at 82%");
    terminal.expectText("[ERROR] net: upstream timeout (retry 1)");
    terminal.expectText("lines:    6");
    terminal.expectText("TAIL:ON");
    terminal.marker("Initial log buffer");
    terminal.sleep(400);

    // Append two new log entries using A.
    terminal.typeText("a");
    terminal.waitForText("lines:    7");
    terminal.expectText("appended: 1");
    terminal.expectText("worker: batch #42 committed");
    terminal.sleep(300);

    terminal.typeText("a");
    terminal.waitForText("lines:    8");
    terminal.expectText("appended: 2");
    terminal.expectText("mem: arena utilization at 86%");
    terminal.marker("Appended streaming entries");
    terminal.sleep(400);

    // Toggle followTail with F.
    terminal.typeText("f");
    terminal.waitForText("TAIL:OFF");
    terminal.sleep(300);

    terminal.typeText("f");
    terminal.waitForText("TAIL:ON");
    terminal.sleep(300);

    // Clear the buffer with C, then append a fresh line.
    terminal.typeText("c");
    terminal.waitForText("lines:    0");
    terminal.expectText("last:     cleared");
    terminal.sleep(300);

    terminal.typeText("a");
    terminal.waitForText("lines:    1");
    terminal.sleep(300);
}
