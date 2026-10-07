// An idle application must not keep rendering. terminal.sleep() runs the event
// loop without requesting a render, so every frame committed during the sleep
// was scheduled by the application itself. The file manager has no periodic
// timers, so the budget is zero.
function main(terminal) {
    const BUDGET = 0;
    terminal.waitForText("CURRENT", 2000);
    terminal.sleep(300);            // let startup settle
    const before = terminal.frameCount();
    terminal.sleep(500);
    const advanced = terminal.frameCount() - before;
    if (advanced > BUDGET)
        terminal.fail(`idle frame budget exceeded: ${advanced} frames in 500 ms (budget ${BUDGET}); `
                  + JSON.stringify(terminal.frameStats()));
}
