/** @param {Runner} app */
function main(app) {
    app.expectText("Hello, QmlTui!");
    app.expectText("Interactions: 0");
    app.sleep(900);

    app.press("Enter");
    app.expectText("Interactions: 1");
    app.sleep(500);

    app.press("Enter");
    app.expectText("Interactions: 2");
    app.sleep(500);

    app.press("Enter");
    app.expectText("Interactions: 3");
    app.sleep(700);

    app.press("r");
    app.expectText("Interactions: 0");
    app.sleep(900);
}
