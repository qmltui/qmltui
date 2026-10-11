/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("apple");
    terminal.expectText("pear");
    terminal.captureText("grid_view.txt");
}
