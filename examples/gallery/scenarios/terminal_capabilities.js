/** @param {Runner} app */
function main(app) {
    app.waitForText("Host Terminal Feature Detection & Diagnostics");
    app.expectText("Terminal Program:");
    app.expectText("Color Resolution:");
    app.expectText("Synchronized Output:");
    app.expectText("Kitty Keyboard:");
    app.expectText("Kitty Graphics:");
    app.expectText("Sixel Graphics:");
    app.expectText("Default Foreground:");
    app.expectText("Default Background:");
    app.expectText("ANSI Palette");
    app.expectText("black");
    app.expectText("red");
    app.expectText("blue");
    app.expectText("white");
    app.captureText("capabilities.txt");
}
