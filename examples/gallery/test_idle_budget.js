/** @param {Runner} app */
function main(app) {
    app.expectText("QmlTui Gallery");
    app.sleep(300); // Real event-loop settling, then observe unscripted renders.
    const before = app.frameCount();
    app.sleep(500);
    const advanced = app.frameCount() - before;
    if (advanced !== 0)
        app.fail("Gallery repainted while idle: " + advanced);
}
