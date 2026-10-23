function main(app) {
    app.waitForText("ASYNC READY");
    app.expectSnapshot("snapshots/fixture.txt");
}
