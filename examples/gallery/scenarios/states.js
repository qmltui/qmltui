/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("States");
    terminal.expectText("Inspector");
    terminal.expectText("NODE: NOMINAL");
    terminal.expectText("state: \"nominal\"");
    terminal.sleep(450);

    terminal.press("2");
    terminal.expectText("state: \"degraded\"");
    terminal.sleep(350);
    terminal.expectText("NODE: DEGRADED");
    terminal.expectText("bar.width: 20");
    terminal.sleep(450);

    terminal.press("3");
    terminal.expectText("state: \"maintenance\"");
    terminal.sleep(350);
    terminal.expectText("NODE: MAINTENANCE");
    terminal.expectText("bar.width: 28");
    terminal.sleep(450);

    terminal.press("1");
    terminal.expectText("state: \"nominal\"");
    terminal.sleep(350);
    terminal.expectText("bar.width: 10");
    terminal.sleep(400);
}
