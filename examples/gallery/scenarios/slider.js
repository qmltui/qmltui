/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Audio & Channel Mixer");
    terminal.expectText("Volume  42%");
    terminal.expectText("Bass    60%");
    terminal.expectText("Treble  50%");
    terminal.expectText("stepSize: 1");

    // Press RIGHT on focused Volume slider -> 43%
    terminal.press("RIGHT");
    terminal.waitForText("Volume  43%");
    terminal.expectText("volume: 43%");

    // Toggle stepSize: 5 in Inspector
    terminal.clickText("stepSize: 5");
    terminal.waitForText("Step: 5");

    // Tab back or click Bass slider, or press Tab to move focus and adjust
    terminal.press("TAB"); // enabled checkbox
    terminal.press("TAB"); // Reset EQ button
    terminal.press("TAB"); // Volume slider
    terminal.press("RIGHT");
    terminal.waitForText("Volume  48%");

    // Toggle showValue off in Inspector
    terminal.clickText("showValue");
    terminal.expectNoText("[48]");

    // Reset EQ button restores defaults
    terminal.clickText("Reset EQ");
    terminal.waitForText("Volume  42%");
    terminal.expectText("volume: 42%");
}
