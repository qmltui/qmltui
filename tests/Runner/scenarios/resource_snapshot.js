function main(app) {
    app.waitForText("STATUS: ASYNC READY");
    app.expectSnapshot("snapshots/fixture.txt");
}
