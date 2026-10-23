function main(app) {
    app.advanceTime(100);
    app.fail("advanceTime() must fail on the real clock");
}
