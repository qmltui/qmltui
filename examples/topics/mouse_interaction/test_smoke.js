/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Concept Demo: Mouse Interactions & MouseArea");
    terminal.click(10, 10);
    terminal.expectText("Clicked button 1");
    terminal.scroll(10, 10, 0, 1);
    terminal.expectText("Wheel deltaY 1 deltaX 0");
    terminal.advanceTime(200);
    terminal.captureText("mouse_interaction.txt");
}
