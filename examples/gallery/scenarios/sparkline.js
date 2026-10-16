/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Live Metrics Sparkline Monitor");
    terminal.expectText("last: 62%");
    terminal.expectText("max: 72%");
    terminal.expectText("filled: true");

    // Press Space on focused Push Sample button -> pushes 81% CPU sample
    terminal.press("SPACE");
    terminal.waitForText("last: 81%");
    terminal.expectText("max: 81%");
    terminal.expectText("Pushed: 1");

    // Push another sample by clicking Push Sample -> 74% CPU
    terminal.clickText("Push Sample");
    terminal.waitForText("last: 74%");
    terminal.expectText("Pushed: 2");

    // Toggle filled checkbox in Inspector
    terminal.clickText("filled");
    terminal.waitForText("filled: false");
    terminal.expectText("Mode: Line Curve");
    terminal.expectText("OUTLINE");
}
