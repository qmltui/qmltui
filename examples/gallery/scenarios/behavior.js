/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Behavior");
    terminal.expectText("Inspector");
    terminal.expectText("Behavior on x");
    terminal.expectText("Easing.OutBounce");
    terminal.sleep(450);

    terminal.press("Right");
    terminal.sleep(400);
    terminal.expectText("puck.x: 14 -> 14");
    terminal.expectText("bar.width: 24 -> 24");
    terminal.sleep(400);

    terminal.press("e");
    terminal.expectText("Easing.OutCubic");
    terminal.sleep(350);

    terminal.press("Space");
    terminal.sleep(400);
    terminal.expectText("puck.x: 26 -> 26");
    terminal.expectText("bar.width: 16 -> 16");
    terminal.sleep(400);

    terminal.press("Left");
    terminal.sleep(400);
    terminal.expectText("puck.x: 14 -> 14");
    terminal.sleep(350);
}
