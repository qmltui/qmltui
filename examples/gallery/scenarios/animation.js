/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Animation");
    terminal.expectText("Inspector");
    terminal.expectText("Easing.Linear");
    terminal.expectText("Easing.OutBounce");
    terminal.sleep(600);

    terminal.expectText("Linear:     28 / 28");
    terminal.expectText("OutBounce:  28 / 28");
    terminal.sleep(400);

    terminal.press("Space");
    terminal.expectText("direction: Contract");
    terminal.sleep(600);

    terminal.expectText("Linear:     6 / 28");
    terminal.sleep(400);

    terminal.clickText("Play / Flip [Space]");
    terminal.expectText("direction: Expand");
    terminal.sleep(600);
    terminal.expectText("Linear:     28 / 28");
}
