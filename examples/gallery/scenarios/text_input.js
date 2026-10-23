/** @param {Runner} app */
function main(app) {
    app.expectText("Hello, stranger!");
    app.sleep(700);
    app.typeText("Ada");
    app.expectText("Hello, Ada!");
    app.sleep(700);
    app.press("Tab");
    app.typeText("s3cr3t");
    app.expectText("Password has 6 characters.");
    app.sleep(900);
    app.captureText("text_input.txt");
    app.stopRecording();
}
