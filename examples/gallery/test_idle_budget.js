/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui Gallery");
    terminal.sleep(300); // Real event-loop settling, then observe unscripted renders.
    const before = terminal.frameCount();
    terminal.sleep(500);
    const advanced = terminal.frameCount() - before;
    if (advanced !== 0)
        terminal.fail("Gallery repainted while idle: " + advanced);
}
