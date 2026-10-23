/** @param {Runner} app */
function main(app) {
    app.expectText("RowLayout");
    app.expectText("GridLayout");
    app.expectText("0,2 span 2");
    app.sleep(1200);
    app.captureText("layouts.txt");
    app.stopRecording();
}
