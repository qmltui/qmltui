/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Layouts Dashboard Stage");
    terminal.expectText("spacing: 1");
    terminal.expectText("sidebarWidth: 9");
    terminal.sleep(300);

    terminal.press("g");
    terminal.expectText("spacing: 2");
    terminal.sleep(300);

    terminal.press("g");
    terminal.expectText("spacing: 0");
    terminal.sleep(300);

    terminal.press("s");
    terminal.expectText("sidebarWidth: 14");
    terminal.expectText("Sidebar: Wide");
    terminal.sleep(300);
}
