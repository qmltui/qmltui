/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Concept Demo: Mouse Interactions & MouseArea");
    terminal.advanceTime(200);
    terminal.captureText("mouse_interaction.txt");
}
