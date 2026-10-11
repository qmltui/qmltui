/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui ScrollView Component Example (Use Up/Down/PageUp/PageDown/Wheel ");
    terminal.advanceTime(200);
    terminal.captureText("scroll_view.txt");
}
