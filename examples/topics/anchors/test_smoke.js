/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Robust Anchoring System Demo");
    terminal.advanceTime(200);
    terminal.captureText("anchors.txt");
}
