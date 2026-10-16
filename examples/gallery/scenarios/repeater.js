/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Repeater Node Matrix");
    terminal.expectText("Total Nodes: 4");
    terminal.expectText("node-04");
    terminal.sleep(300);

    terminal.press("a");
    terminal.expectText("Total Nodes: 5");
    terminal.expectText("node-05");
    terminal.sleep(300);

    terminal.press("d");
    terminal.expectText("Total Nodes: 4");
    terminal.sleep(300);

    terminal.press("c");
    terminal.expectText("Accent: Cyan");
    terminal.sleep(300);
}
