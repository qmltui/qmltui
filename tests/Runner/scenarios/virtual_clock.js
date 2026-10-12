// Deterministic time under QMLTUI_RUNNER_CLOCK=virtual: QML Timer, a QmlTui
// animation mid-state, frame discipline, and virtual sleep().
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("STATUS: READY");
    if (terminal.now() !== 0)
        terminal.fail("Virtual time must start at 0, got " + terminal.now());

    // The fixture's 75 ms one-shot QML Timer fires on the virtual timeline.
    terminal.advanceTime(74);
    terminal.expectText("STATUS: READY");
    terminal.advanceTime(1);
    terminal.expectText("STATUS: ASYNC READY");

    // A repeating 100 ms QML Timer ticks exactly five times in 500 ms.
    terminal.press("T");
    terminal.expectText("TICKS: 0");
    terminal.advanceTime(500);
    terminal.expectText("TICKS: 5");
    terminal.press("T");

    // A 400 ms QmlTui NumberAnimation holds a committed mid-state at 200 ms
    // and its end state after the full duration.
    terminal.press("A");
    terminal.advanceTime(200);
    const bar = terminal.itemRect("bar");
    if (bar.width <= 4 || bar.width >= 24)
        terminal.fail("Animation mid-state expected, width " + bar.width);
    terminal.expectStyle(41, 20, { background: "#E06C75FF" });
    terminal.expectStyle(63, 20, { background: "#101820FF" });
    terminal.advanceTime(400);
    if (terminal.itemRect("bar").width !== 24)
        terminal.fail("Animation must finish at width 24");
    terminal.expectStyle(63, 20, { background: "#E06C75FF" });

    // Nothing scheduled: advancing time commits no frame.
    const before = terminal.frameCount();
    terminal.advanceTime(300);
    if (terminal.frameCount() !== before)
        terminal.fail("advanceTime() with nothing scheduled must not commit frames");

    // sleep() advances virtual time instead of waiting.
    const t0 = terminal.now();
    terminal.sleep(250);
    if (terminal.now() - t0 !== 250)
        terminal.fail("sleep() must advance exactly 250 virtual ms");

    terminal.captureText("virtual_clock.txt");
}
