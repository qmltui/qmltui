/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("REAL-TIME CLUSTER & TELEMETRY MONITOR");
    terminal.sleep(3000);
    terminal.stopRecording();
}
