function main(app) {
    app.waitForText("ASYNC READY");
    app.captureStyled("styled.txt");
    app.expectStyled("snapshots/fixture_styled.txt");
}
