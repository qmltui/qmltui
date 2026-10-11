/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("REAL-TIME CLUSTER & TELEMETRY MONITOR");
    terminal.advanceTime(200);
    terminal.captureText("sparkline.txt");
}
