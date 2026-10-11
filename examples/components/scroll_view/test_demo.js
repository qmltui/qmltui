/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui ScrollView Component Example");
    terminal.sleep(1500);
    terminal.press("Down");
    terminal.sleep(400);
    terminal.press("Down");
    terminal.sleep(400);
    terminal.press("Down");
    terminal.sleep(400);
    terminal.press("Down");
    terminal.sleep(400);
    terminal.press("Down");
    terminal.sleep(400);
    terminal.press("Down");
    terminal.sleep(1000);
    terminal.press("Up");
    terminal.sleep(400);
    terminal.press("Up");
    terminal.sleep(400);
    terminal.press("Up");
    terminal.sleep(1500);
    terminal.stopRecording();
}
