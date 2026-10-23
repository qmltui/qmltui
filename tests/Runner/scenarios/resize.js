// resize() commits a frame at the new dimensions: bound content reflows, the
// right-anchored marker moves, and size() reports the committed frame.
function main(app) {
    app.waitForText("SIZE: 80x24");

    const initial = app.size();
    if (initial.columns !== 80 || initial.rows !== 24)
        app.fail("size() expected 80x24, got " + initial.columns + "x" + initial.rows);
    if (app.findText("EDGE")[0].x !== 75)
        app.fail("EDGE marker is not anchored to the right at 80 columns");

    app.resize(40, 16);
    app.expectText("SIZE: 40x16");
    app.expectTextInRegion(35, 1, 5, 1, "EDGE");
    const shrunk = app.size();
    if (shrunk.columns !== 40 || shrunk.rows !== 16)
        app.fail("size() expected 40x16 after resize");

    app.resize(120, 40);
    app.expectText("SIZE: 120x40");
    if (app.findText("EDGE")[0].x !== 115)
        app.fail("EDGE marker did not follow the resize to 120 columns");
    app.captureText("resize.txt");
}
