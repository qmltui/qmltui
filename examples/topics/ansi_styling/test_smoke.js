/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Concept Demo: Text Styling & ANSI Parser");
    terminal.advanceTime(200);
    terminal.captureText("ansi_styling.txt");
}
