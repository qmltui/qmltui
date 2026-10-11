/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Layout and text");
    terminal.resize(60, 20);
    terminal.expectText("source text.");
    terminal.expectText("Ready to build");
    terminal.captureText("tutorial.txt");
}
