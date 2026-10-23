// Deterministic time under QMLTUI_RUNNER_CLOCK=virtual: QML Timer, a QmlTui
// animation mid-state, frame discipline, and virtual sleep().
/** @param {Runner} app */
function main(app) {
    app.waitForText("STATUS: READY");
    if (app.now() !== 0)
        app.fail("Virtual time must start at 0, got " + app.now());

    // The fixture's 75 ms one-shot QML Timer fires on the virtual timeline.
    app.advanceTime(74);
    app.expectText("STATUS: READY");
    app.advanceTime(1);
    app.expectText("STATUS: ASYNC READY");

    // A repeating 100 ms QML Timer ticks exactly five times in 500 ms.
    app.press("T");
    app.expectText("TICKS: 0");
    app.advanceTime(500);
    app.expectText("TICKS: 5");
    app.press("T");

    // A 400 ms QmlTui NumberAnimation holds a committed mid-state at 200 ms
    // and its end state after the full duration.
    app.press("A");
    app.advanceTime(200);
    const bar = app.itemRect("bar");
    if (bar.width <= 4 || bar.width >= 24)
        app.fail("Animation mid-state expected, width " + bar.width);
    app.expectStyle(41, 20, { background: "#E06C75FF" });
    app.expectStyle(63, 20, { background: "#101820FF" });
    app.advanceTime(400);
    if (app.itemRect("bar").width !== 24)
        app.fail("Animation must finish at width 24");
    app.expectStyle(63, 20, { background: "#E06C75FF" });

    // Nothing scheduled: advancing time commits no frame.
    const before = app.frameCount();
    app.advanceTime(300);
    if (app.frameCount() !== before)
        app.fail("advanceTime() with nothing scheduled must not commit frames");

    // sleep() advances virtual time instead of waiting.
    const t0 = app.now();
    app.sleep(250);
    if (app.now() - t0 !== 250)
        app.fail("sleep() must advance exactly 250 virtual ms");

    app.captureText("virtual_clock.txt");
}
