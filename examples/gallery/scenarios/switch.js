/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Wi-Fi on, Bluetooth off");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Space");
    terminal.expectText("Wi-Fi on, Bluetooth on");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Space");
    terminal.expectText("Radios are off while airplane mode is on");
    terminal.sleep(900);
    terminal.captureText("switch.txt");
    terminal.stopRecording();
}
