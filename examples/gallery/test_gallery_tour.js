/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Gallery");
    terminal.marker("Gallery");
    terminal.captureText("gallery/grid.txt");
    terminal.sleep(1500);
    const catalog = terminal.rootObject().catalog;
    for (let i = 0; i < catalog.length; ++i) {
        terminal.press("Enter");
        terminal.expectText("back to grid");
        terminal.expectNoText("✗ Error");
        terminal.marker(catalog[i].name);
        terminal.sleep(1000);
        terminal.captureText("gallery/page_" + i + ".txt");
        terminal.press("Escape");
        terminal.expectText("QmlTui Gallery");
        terminal.press("Right");
    }
    terminal.marker("Tour complete");
    terminal.sleep(1500);
    terminal.stopRecording();
}
