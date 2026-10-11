/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Branding & Logo Terminal Showcase");
    terminal.waitForText("Mark (Dark)");

    // Capture initial vector mark
    terminal.captureText("branding_logos/initial.txt");

    // Cycle render modes with 'M'
    terminal.press("M");
    terminal.waitForText("HalfBlock");
    terminal.press("M");
    terminal.waitForText("Braille");
    terminal.press("M");
    terminal.waitForText("ASCII");

    // Toggle theme with 'T'
    terminal.press("T");
    terminal.waitForText("Theme: Light");
    terminal.press("T");
    terminal.waitForText("Theme: Dark");

    // Navigate list with Down key
    terminal.press("Down");
    terminal.waitForText("Lockup Wordmark (Dark)");

    // Quick jump to ANSI cells (key 7 selects index 6: ANSI Half-Blocks 24x12)
    terminal.press("7");
    terminal.waitForText("ANSI Half-Blocks 24x12");
    terminal.waitForText("Terminal Cells");

    // Navigate to Mascot banner (index 12: 6 presses from index 6)
    terminal.press("Down");
    terminal.press("Down");
    terminal.press("Down");
    terminal.press("Down");
    terminal.press("Down");
    terminal.press("Down");
    terminal.waitForText("ASCII Art");

    terminal.captureText("branding_logos/mascot.txt");

    terminal.finish();
}
