// An idle application must not keep rendering. terminal.sleep() runs the event
// loop without requesting a render, so every frame committed during the sleep
// was scheduled by the application itself. btop has a 1 s clock timer and a
// periodic model refresh, so at most two frames may legitimately land in a
// 500 ms window.
function main(terminal) {
    const BUDGET = 2;
    terminal.waitForText("PROCESSES", 2000);
    terminal.sleep(300);            // let startup settle
    const before = terminal.frameCount();
    terminal.sleep(500);
    const advanced = terminal.frameCount() - before;
    if (advanced > BUDGET)
        terminal.fail(`idle frame budget exceeded: ${advanced} frames in 500 ms (budget ${BUDGET}); `
                  + JSON.stringify(terminal.frameStats()));
}
