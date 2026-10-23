/** @param {Runner} app */
function main(app) {
    app.expectText("QmlTui Gallery");
    app.marker("Gallery");
    app.captureText("gallery/grid.txt");
    app.sleep(1500);
    const catalog = app.rootObject().catalog;
    for (let i = 0; i < catalog.length; ++i) {
        app.press("Enter");
        app.expectText("back to grid");
        app.expectNoText("✗ Error");
        app.marker(catalog[i].name);
        app.sleep(1000);
        app.captureText("gallery/page_" + i + ".txt");
        app.press("Escape");
        app.expectText("QmlTui Gallery");
        app.press("Right");
    }
    app.marker("Tour complete");
    app.sleep(1500);
    app.stopRecording();
}
