/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("ScrollView Blueprint");
    terminal.expectText("contentY: 0");
    terminal.expectText("contentHeight: 28");
    terminal.expectText("00 │ [EDGE]");
    terminal.sleep(300);

    terminal.press("Down");
    terminal.press("Down");
    terminal.expectText("contentY: 2");
    terminal.sleep(300);

    terminal.clickText("Mid");
    terminal.expectText("contentY: 8");
    terminal.sleep(300);

    terminal.clickText("End");
    terminal.expectText("27 │ [DISK]");
    terminal.sleep(300);

    terminal.clickText("Top");
    terminal.expectText("contentY: 0");
    terminal.sleep(300);
}
