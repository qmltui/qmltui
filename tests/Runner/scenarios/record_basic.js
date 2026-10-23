// A RECORD scenario: the cast opens with the first frame, carries later output
// and the markers placed between steps.
/** @param {Runner} app */
function main(app) {
    app.expectText("TESTRUNNER FIXTURE");
    app.press("Tab");
    app.marker("start");
    app.press("Tab");            // focus the fixture's TextInput
    app.typeText("cast");
    app.expectText("INPUT: cast");
    app.sleep(50);
    app.marker("typed");
    app.resize(90, 26);
    app.expectText("SIZE: 90x26");
}
