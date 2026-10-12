/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui ToolTip Component Example");
    terminal.advanceTime(200);
    terminal.captureText("tool_tip.txt");
}
