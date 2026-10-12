/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("ASCIIFY FLUID BACKGROUND");
    terminal.expectText("Procedural domain-warping fluid field");

    // Advance virtual clock to verify time integration
    terminal.advanceTime(200);

    // Click to stir fluid wake
    terminal.click(20, 10);
    terminal.advanceTime(100);

    // Switch charset with keypress 's'
    terminal.press("s");
    terminal.advanceTime(100);

    terminal.captureText("fluid.txt");
    terminal.press("q");
}
