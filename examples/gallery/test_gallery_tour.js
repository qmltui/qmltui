/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Gallery");
    terminal.marker("Gallery");
    terminal.captureText("gallery/grid.txt");
    terminal.sleep(1500);
    const catalog = terminal.rootObject().catalog;
    for (let i = 0; i < catalog.length - 1; ++i) {
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
    // Inline mode is exercised by test_gallery.js; end this full-screen tour
    // before switching buffers so GIF and SVG renderers agree on the result.
    terminal.stopRecording();
}
