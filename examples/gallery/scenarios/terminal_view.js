/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("DUAL TERMINAL WORKFLOW");
    terminal.waitForText("[RUNNING]");

    terminal.press("F1");
    terminal.typeText("printf 'QMLTUI_%s\\n' LEFT_READY");
    terminal.press("Enter");
    terminal.waitForText("QMLTUI_LEFT_READY");

    terminal.press("F2");
    terminal.typeText("printf 'QMLTUI_%s\\n' RIGHT_READY");
    terminal.press("Enter");
    terminal.waitForText("QMLTUI_RIGHT_READY");
    terminal.expectText("QMLTUI_LEFT_READY");

    terminal.press("F4");
    terminal.waitForText("Focused Command Input");
    terminal.press("Enter");
    terminal.waitForCondition(frame => frame.split("\n").slice(2, 13)
        .some(line => line.includes("Hello QmlTui Dual Terminal!")));
    terminal.waitForText("Hello QmlTui Dual Terminal!");
    terminal.captureText("terminal_view.txt");
}
