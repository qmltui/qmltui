/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Declarative TUI Animation & Transition Demo");
    terminal.advanceTime(200);
    terminal.captureText("animation.txt");
}
