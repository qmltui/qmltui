/** @param {Runner} app */
function main(app) {
    app.expectText("Inline mode");
    app.expectScrollbackText("QmlTui");
    app.expectScrollbackText("Inline Agent CLI");
    app.expectText("Enter a message.");
    app.typeText("hello inline");
    app.expectText("hello inline");
    app.captureText("typed.txt");
    app.press("Enter");
    app.expectText("printed: 1");
    app.expectNoText("hello inline");            // gone from the live region...
    app.expectScrollbackText("hello inline");    // ...and committed to the scrollback
    app.captureScrollbackStyled("scrollback_styled.txt");
    app.expectText("Enter a message.");
    app.typeText("clear me");
    app.press("Escape");
    app.expectNoText("clear me");
    app.captureText("cleared.txt");
}
