function main(app) {
    app.waitForText("ASYNC READY");
    app.expectSnapshot("snapshots/mismatch.txt");
}
