/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("PARAGRAPH HYPERTEXT LINKS DEMO");
    terminal.advanceTime(200);
    terminal.captureText("paragraph.txt");
}
